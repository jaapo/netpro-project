#include "protomsg.h"
#include "fap.h"
#include "fctp.h"
#include "dcp.h"
#include "fsservd.h"
#include "util.h"
#include "netutil.h"

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <pthread.h>

#define SYSLOGPRIO (LOG_USER | LOG_ERR)

#define MAX_CLIENTS 100

static char *conffile;

char *dirserver;
char *dataloc;
char *maxspace;

static int listensd;

static struct addrinfo *dserv_ai;
static int dssd;

uint64_t sid;
uint64_t fsid;

static uint64_t lastcid = 1;
static int clicnt = 0;

static pthread_t fctp_thread;

static struct client_info clients[MAX_CLIENTS];

int main(int argc, char* argv[], char* envp[]) {
	srandom(2);
	conffile = read_args(argc, argv, "/etc/dfs_fileserv.conf");

	add_config_param("directory_server", &dirserver);
	add_config_param("data_location", &dataloc);
	add_config_param("maximum_space", &maxspace);

	read_config(conffile);
	
	go_daemon();
	pthread_create(&fctp_thread, NULL, fctp_server, dataloc);

	dserv_ai = get_server_address(dirserver, DCPPORTSTR, NULL);
	connect_dirserv();

	listensd = start_listen(FAPPORT);
	do_fap_server();

	return 0;
}

void go_daemon() {

}

void connect_dirserv() {
	int ret;
	struct addrinfo *ai;
	ai = dserv_ai;

	while (ai) {
		char servaddr[NI_MAXHOST];
		char servport[NI_MAXSERV];
		ret = getnameinfo(ai->ai_addr, ai->ai_addrlen, servaddr, NI_MAXHOST, servport, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

		if (ret != 0) {
			SYSLOG(SYSLOGPRIO, "getnameinfo error: %s", gai_strerror(ret));
			servaddr[0] = '\0';
			servport[0] = '\0';
		}
		
		printf("connecting to directory server %s (%s:%s)...\n", dirserver, servaddr, servport);
		SYSLOG(SYSLOGPRIO, "connecting to directory server %s (%s:%s)", dirserver, servaddr, servport);
		
		//XXX initial values
		dssd = dcp_open(ai, &sid, atoi(maxspace), 0, 0);
		if (dssd >= 0) break;
		printf("connection failed\n");
		SYSLOG(SYSLOGPRIO, "directory server connection to %s failed", dirserver);

		ai = ai->ai_next;
	}

	syscallerr(dssd, "dcp-connection failed %s", dirserver);	
}

//TODO: threads
void do_fap_server() {
	int clisd, ret;
	struct client_info *clinfo;
	uint64_t client_id = nextcid();
	struct sockaddr *cliaddr = NULL;
	socklen_t cliaddrlen;

	for (;;) {
		NO_INTR(clisd = accept(listensd, cliaddr, &cliaddrlen));
		if (clisd < 0 && errno == EINTR) continue;
		syscallerr(clisd, "%s: accept() failed", __func__);
		
		ret = register_client(clisd, client_id);
		if (ret < 0) {
			fprintf(stderr, "can't accept more clients");
			close(clisd);
			continue;
		}
		clinfo = &clients[ret];

		ret = fap_accept(clinfo);
		if (ret < 0) {
			fprintf(stderr, "error accepting client\n");
			close(clisd);
			continue;
		}

		SYSLOG(SYSLOGPRIO, "connection from %s by user %s", clinfo->host, clinfo->user);
		serve_client(clinfo);
	}
}

void serve_client(struct client_info *info) {
	struct fsmsg *msg;
	int ret;
	uint16_t cmd;

	for(;;) {
		msg = fsmsg_from_socket(info->sd, FAP);
		if (!msg) return;
		ret = fap_validate_sections(msg);
		if (ret < 0) {
			fap_send_error(info->sd, msg->tid, info->id, ERR_MSG, "");
			continue;
		}

		info->lasttid = msg->tid;

		DEBUGPRINT("received message %hd from client %ld, socket %d", msg->msg_type, info->id, info->sd);

		switch ((enum fap_type) msg->msg_type) {
			case FAP_QUIT:
				DEBUGPRINT("client %ld quit", info->id);
				fap_send_ok(info->sd, msg->tid, info->id);
				close(info->sd);
				info->id = 0;
				info->sd = 0;
				return;
				break;
			case FAP_COMMAND:
				cmd = SECI(msg, 0);
				switch ((enum fap_commands) cmd) {
					case FAP_CMD_CREATE:
						create_file(info, SECSDUP(msg, 1));
						break;
					case FAP_CMD_OPEN:
						break;
					case FAP_CMD_CLOSE:
						break;
					case FAP_CMD_STAT:
						break;
					case FAP_CMD_READ:
						serve_file(info, SECSDUP(msg, 1));
						break;
					case FAP_CMD_WRITE:
						write_file(info, SECSDUP(msg, 1), SECI(msg, 2));
						break;
					case FAP_CMD_DELETE:
						break;
					case FAP_CMD_COPY:
						break;
					case FAP_CMD_FIND:
						break;
					case FAP_CMD_LIST:
						list_directory(info, SECI(msg, 1), SECS(msg, 2).data, SECS(msg, 2).length);
						break;
				}
				break;
			case FAP_RESPONSE:
				break;
			case FAP_ERROR:
				break;
			default:
				DEBUGPRINT("%s", "unexpected message");
		}
	}
	
	DEBUGPRINT("%s reached end", __func__);
}

int register_client(int clisd, uint64_t cid) {
	int i;
	if (cid == 0 || clicnt > MAX_CLIENTS) return -1;
	for (i=0;i<MAX_CLIENTS;i++) {
		if (clients[i].id == 0) {
			clients[i].id = cid;
			clients[i].lasttid = 0;
			clients[i].sd = clisd;
			clients[i].datasd = 0;
			clients[i].dataport = FAP_DATAPORT_BASE + i;
			clicnt++;
			break;
		}
	}
	return i;
}

uint64_t nextcid() {
	return ++lastcid;
}

void list_directory(struct client_info *info, int recurse, char *path, int pathlen) {
	struct fsmsg *dirmsg, *climsg;
	union section_data data;
	int ret;
	uint64_t tid = nexttid();

	dirmsg = dcp_create_msg(tid, fsid, sid, DCP_READ);
	data.integer = recurse;
	fsmsg_add_section(dirmsg, ST_INTEGER, &data);
	data.string.data = path;
	data.string.length = pathlen;
	fsmsg_add_section(dirmsg, ST_STRING, &data);
	fsmsg_add_section(dirmsg, ST_NONEXT, NULL);

	ret = fsmsg_send(dssd, dirmsg, DCP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, dssd);
	fsmsg_free(dirmsg);

	dirmsg = fsmsg_from_socket(dssd, DCP);
	
	ret = dcp_check_response(dirmsg, tid, sid, fsid, DCP_READ);
	if (ret < 0) {
		fprintf(stderr, "invalid response to DCP_READ, error: %d\n", ret);
		return;
	}
	
	ret = dcp_validate_sections(dirmsg);
	if (ret < 0) {
		fprintf(stderr, "section validation failed, error: %d\n", ret);
		return;
	}

	climsg = fap_create_msg(info->lasttid, sid, info->id, fsid, FAP_RESPONSE);
	data.integer = FAP_FILEINFO;
	fsmsg_add_section(climsg, ST_INTEGER, &data);
	fsmsg_add_section(climsg, ST_INTEGER, &dirmsg->sections[0]->data);
	
	for (int i=0;i<SECI(dirmsg, 0);i++) {
		fsmsg_add_section(climsg, ST_FILEINFO, &dirmsg->sections[i+1]->data);
	}

	fsmsg_add_section(climsg, ST_NONEXT, NULL);
	
	ret = fsmsg_send(info->sd, climsg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, info->sd);
	
	fsmsg_free(dirmsg);
	fsmsg_free(climsg);
}

void create_file(struct client_info *info, char *path) {
	struct fsmsg *dirmsg, *climsg;
	dirmsg = NULL;
	climsg = NULL;
	union section_data data;
	int ret;
	uint64_t tid = nexttid();

	//send create to directory
	dirmsg = dcp_create_msg(tid, fsid, sid, DCP_CREATE);

	memset(&data, '\0', sizeof(data));
	data.fileinfo.path = path;
	data.fileinfo.pathlen = strlen(path);
	data.fileinfo.username = info->user;
	data.fileinfo.usernamelen = strlen(info->user);
	fsmsg_add_section(dirmsg, ST_FILEINFO, &data);
	fsmsg_add_section(dirmsg, ST_NONEXT, NULL);

	ret = fsmsg_send(dssd, dirmsg, DCP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, dssd);
	fsmsg_free(dirmsg);

	dirmsg = fsmsg_from_socket(dssd, DCP);
	
	//check for exists error
	ret = dcp_check_response(dirmsg, tid, sid, fsid, DCP_CREATE);

	if (dirmsg && ret == -5 && SECI(dirmsg, 0) == ERR_EXISTS) {
		fap_send_error(info->sd, info->lasttid, info->id, ERR_EXISTS, "file already exists");
		goto cleanup;
	} else if (ret < 0) {
		fprintf(stderr, "invalid response to DCP_CREATE, error: %d\n", ret);
		goto cleanup;
	}
	
	ret = dcp_validate_sections(dirmsg);
	if (ret < 0) {
		fprintf(stderr, "section validation failed, error: %d\n", ret);
		goto cleanup;
	}

	if (SECI(dirmsg, 0) != 1) {
		fprintf(stderr, "expected only one fileinfo section, received message with %d\n", SECI(dirmsg, 0));
	}

	//create local file
	char *localpath = malloc(strlen(dataloc) + strlen(path) + 1);
	DEBUGPRINT("dataloc: %s, dirmsg->path: %s", dataloc, path);
	strcpy(localpath, dataloc);
	strcat(localpath, path);
	ret = open(localpath, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
	syscallerr(ret, "unable to create local file %s", localpath);
	close(ret);
	DEBUGPRINT("created local file %s", localpath);
	free(localpath);

	//send response to client
	climsg = fap_create_msg(info->lasttid, sid, info->id, fsid, FAP_RESPONSE);
	data.integer = FAP_FILEINFO;
	fsmsg_add_section(climsg, ST_INTEGER, &data);
	fsmsg_add_section(climsg, ST_INTEGER, &dirmsg->sections[0]->data);	
	fsmsg_add_section(climsg, ST_FILEINFO, &dirmsg->sections[1]->data);
	fsmsg_add_section(climsg, ST_NONEXT, NULL);
	
	ret = fsmsg_send(info->sd, climsg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, info->sd);

cleanup:	
	fsmsg_free(dirmsg);
	fsmsg_free(climsg);
	free(path);
}

void write_file(struct client_info *info, char *path, int length) {
	struct fsmsg *dirmsg, *climsg;
	dirmsg = NULL;
	climsg = NULL;
	union section_data data;
	int ret;
	uint64_t tid = nexttid();

	//send update to directory
	dirmsg = dcp_create_msg(tid, fsid, sid, DCP_UPDATE);

	memset(&data, '\0', sizeof(data));
	data.fileinfo.path = path;
	data.fileinfo.pathlen = strlen(path);
	data.fileinfo.username = info->user;
	data.fileinfo.usernamelen = strlen(info->user);
	fsmsg_add_section(dirmsg, ST_FILEINFO, &data);
	fsmsg_add_section(dirmsg, ST_NONEXT, NULL);

	ret = fsmsg_send(dssd, dirmsg, DCP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, dssd);
	fsmsg_free(dirmsg);

	dirmsg = fsmsg_from_socket(dssd, DCP);
	
	//check for error
	ret = dcp_check_response(dirmsg, tid, sid, fsid, DCP_UPDATE);

	if (ret == -5) {
		fap_send_error(info->sd, info->lasttid, info->id, SECI(dirmsg, 0), "error");
		goto cleanup;
	} else if (ret < 0) {
		fprintf(stderr, "invalid response to DCP_CREATE, error: %d\n", ret);
		goto cleanup;
	}
	
	ret = dcp_validate_sections(dirmsg);
	if (ret < 0) {
		fprintf(stderr, "section validation failed, error: %d\n", ret);
		goto cleanup;
	}

	if (SECI(dirmsg, 0) != 1) {
		fprintf(stderr, "expected only one fileinfo section, received message with %d\n", SECI(dirmsg, 0));
	}

	//update local file
	char *localpath = malloc(strlen(dataloc) + strlen(path) + 1);
	int lfd;
	DEBUGPRINT("dataloc: %s, dirmsg->path: %s", dataloc, path);
	strcpy(localpath, dataloc);
	strcat(localpath, path);
	NO_INTR(ret = open(localpath, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR));
	syscallerr(ret, "unable to create/open local file %s", localpath);
	lfd = ret;
	ret = recvfile(info->datasd, lfd, length);
	syscallerr(ret, "unable to write %d bytes local file %s", length, localpath);
	close(lfd);
	DEBUGPRINT("wrote %d bytes to %s", ret, localpath);
	free(localpath);

	//send response to client
	climsg = fap_create_msg(info->lasttid, sid, info->id, fsid, FAP_RESPONSE);
	data.integer = FAP_OK;
	fsmsg_add_section(climsg, ST_INTEGER, &data);
	fsmsg_add_section(climsg, ST_NONEXT, NULL);
	
	ret = fsmsg_send(info->sd, climsg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, info->sd);

cleanup:	
	fsmsg_free(dirmsg);
	fsmsg_free(climsg);
	free(path);
}

void serve_file(struct client_info *info, char *path) {
	struct fsmsg *climsg;
	climsg = NULL;
	struct stat st;
	memset(&st, 0, sizeof(st));
	union section_data data;
	int ret, fd;

	char *localpath = malloc(strlen(dataloc) + strlen(path) + 1);
	strcpy(localpath, dataloc);
	strcat(localpath, path);

	NO_INTR(ret = open(localpath, O_RDONLY));
	if (ret < 0 && errno == ENOENT) {
		ask_file(path);
		NO_INTR(ret = open(localpath, O_RDONLY));
	}
	syscallerr(ret, "unable to open file %s for reading", localpath);

	fd = ret;
	ret = fstat(fd, &st);
	syscallerr(ret, "unable to fstat() file %s", localpath);
	free(localpath);

	climsg = fap_create_msg(info->lasttid, sid, info->id, fsid, FAP_RESPONSE);
	data.integer = FAP_DATAOUT;
	fsmsg_add_section(climsg, ST_INTEGER, &data);
	data.integer = st.st_size;
	fsmsg_add_section(climsg, ST_INTEGER, &data);
	fsmsg_add_section(climsg, ST_NONEXT, NULL);
	
	ret = fsmsg_send(info->sd, climsg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, info->sd);

	sendfile(info->datasd, fd, NULL, st.st_size);
	close(fd);
	
	fsmsg_free(climsg);
	free(path);
}

void ask_file(char *path) {
	int ret;
	struct fsmsg *msg;
	uint64_t tid = nexttid();
	union section_data data;
	memset(&data, 0, sizeof(data));

	msg = dcp_create_msg(tid, sid, fsid, DCP_GET_REPLICAS);
	fsmsg_add_section(msg, ST_STRING, &data);
	fsmsg_add_section(msg, ST_NONEXT, NULL);
	
	ret = fsmsg_send(dssd, msg, DCP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, dssd);

	fsmsg_free(msg);

	msg = fsmsg_from_socket(dssd, DCP);
	
	//check for error
	ret = dcp_check_response(msg, tid, sid, fsid, DCP_GET_REPLICAS);

	if (ret < 0) {
		fprintf(stderr, "invalid response to DCP_GET_REPLICAS, error: %d\n", ret);
		fsmsg_free(msg);
		return;
	}
	
	ret = dcp_validate_sections(msg);
	if (ret < 0) {
		fprintf(stderr, "invalid response (sections) to DCP_GET_REPLICAS, error: %d\n", ret);
		fsmsg_free(msg);
		return;
	}

	for(int i=0;i<SECI(msg, 1);i++) {
		if (fctp_get_file(SECSDUP(msg, i+1), path, dataloc)) {
			break;
		}
	}

	fsmsg_free(msg);
}
