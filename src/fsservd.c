#include "protomsg.h"
#include "fap.h"
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

static struct client_info clients[MAX_CLIENTS];

int main(int argc, char* argv[], char* envp[]) {
	conffile = read_args(argc, argv, "/etc/dfs_fileserv.conf");

	add_config_param("directory_server", &dirserver);
	add_config_param("data_location", &dataloc);
	add_config_param("maximum_space", &maxspace);

	read_config(conffile);
	
	go_daemon();
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

	//fap_init_server();

	for (;;) {
		clisd = accept(listensd, cliaddr, &cliaddrlen);
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
						break;
					case FAP_CMD_WRITE:
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
