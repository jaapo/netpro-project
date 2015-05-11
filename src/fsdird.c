#include "fsdird.h"
#include "util.h"
#include "netutil.h"
#include "dcp.h"
#include "protomsg.h"

#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define SYSLOGPRIO (LOG_USER | LOG_ERR)

#define MAX_FSERVERS 100

static char *conffile;

static char *dataloc;
static int minreplicas;

static int listensd;

static uint64_t lastsid = 0;

uint64_t fsid = 1333333337;

static struct fileserv_info fileservers[MAX_FSERVERS];
static int fsrvcnt = 0;

static struct node *dirroot;

int main(int argc, char* argv[], char* envp[]) {
	srandom(1);
	char *tmp;

	conffile = read_args(argc, argv, "/etc/dfs_dirserv.conf");

	add_config_param("data_location", &dataloc);
	add_config_param("min_replicas", &tmp);
	read_config(conffile);

	minreplicas = atoi(tmp);

	dirroot = dir_init();
	add_node("/lol");
	add_node("/asd");

	go_daemon();

	listensd = start_listen(DCPPORT);
	do_dcp_server();

	return 0;
}

void go_daemon() {

}

void do_dcp_server() {
	int fsrvsd, ret;
	struct fileserv_info *fsrvinfo;
	uint64_t server_id = nextsid();
	struct sockaddr *fssrvaddr = NULL;
	socklen_t fssrvaddrlen;

	for (;;) {
		fsrvsd = accept(listensd, fssrvaddr, &fssrvaddrlen);
		if (fsrvsd < 0 && errno == EINTR) continue;
		syscallerr(fsrvsd, "%s: accept() failed", __func__);
		
		ret = register_fsrv(fsrvsd, server_id);
		if (ret < 0) {
			fprintf(stderr, "can't accept more file servers\n");
			close(fsrvsd);
			continue;
		}
		fsrvinfo = &fileservers[ret];

		ret = dcp_accept(fsrvinfo);
		if (ret < 0) {
			fprintf(stderr, "error accepting connection from server\n");
			close(fsrvsd);
			continue;
		}

		SYSLOG(SYSLOGPRIO, "connection from %s (server id: %lu)", fsrvinfo->name, fsrvinfo->id);
		serve_fileserver(fsrvinfo);
	}
}

//XXX other also
int register_fsrv(int fsrvsd, uint64_t sid) {
	int i;
	if (sid == 0 || fsrvcnt > MAX_FSERVERS) return -1;
	for (i=0;i<MAX_FSERVERS;i++) {
		if (fileservers[i].id == 0) {
			fileservers[i].id = sid;
			fileservers[i].sd = fsrvsd;
			fsrvcnt++;
			break;
		}
	}
	return i;
}

void serve_fileserver(struct fileserv_info *info) {
	struct fsmsg *msg;
	int ret;

	for(;;) {
		msg = fsmsg_from_socket(info->sd, DCP);
		if (!msg) return;
		ret = dcp_validate_sections(msg);
		if (ret < 0) {
			dcp_send_error(info->sd, msg->tid, info->id, ERR_MSG, "section validation error");
			continue;
		}
		info->lasttid = msg->tid;

		DEBUGPRINT("received message %hd from file server %ld, socket %d", msg->msg_type, info->id, info->sd);

		switch ((enum dcp_msgtype) msg->msg_type) {
			case DCP_READ:
				read_dir(info, SECI(msg, 0), SECSDUP(msg, 1));
				break;
			case DCP_CREATE:
				create_file(info, &SECF(msg, 0));
				break;
			default:
				DEBUGPRINT("%s", "message type serving not implemented yet");
		}
	}
}

uint64_t nextsid() {
	return ++lastsid;
}

void read_dir(struct fileserv_info *info, int recurse, char *path) {
	struct fsmsg *msg;
	union section_data data;
	int ret;
	memset(&data, '\0', sizeof(data));

	msg = dcp_create_msg(info->lasttid, info->id, fsid, DCP_FILEINFO);
	data.integer = 0;
	fsmsg_add_section(msg, ST_INTEGER, &data);

	struct node *n = dir_find_node(path, 1);
	n = n->children;
	while(n) {
		SECI(msg, 0)++;
		data.fileinfo.path = n->name;
		data.fileinfo.pathlen = strlen(n->name);
		data.fileinfo.username = "user";
		data.fileinfo.usernamelen = 4;
		fsmsg_add_section(msg, ST_FILEINFO, &data);
		n=n->next;
	}

	fsmsg_add_section(msg, ST_NONEXT, NULL);
	
	ret = fsmsg_send(info->sd, msg, DCP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, info->sd);

	fsmsg_free(msg);
	free(path);
}

void create_file(struct fileserv_info *info, struct fileinfo_sect *fi) {
	struct fsmsg *msg;
	union section_data data;
	int ret;
	char *path = strndup(fi->path, fi->pathlen);

	struct node *n = dir_find_node(path, 1);
	if (n != NULL) {
		dcp_send_error(info->sd, info->lasttid, info->id, ERR_EXISTS, "file already exists");
		free(path);
		return;
	}
	n = add_node(path);

	msg = dcp_create_msg(info->lasttid, info->id, fsid, DCP_FILEINFO);
	
	memset(&data, '\0', sizeof(data));
	data.integer = 1;
	fsmsg_add_section(msg, ST_INTEGER, &data);

	memset(&data, '\0', sizeof(data));
	data.fileinfo.path = n->name;
	data.fileinfo.pathlen = strlen(n->name);
	data.fileinfo.username = "user";
	data.fileinfo.usernamelen = 4;
	fsmsg_add_section(msg, ST_FILEINFO, &data);
	fsmsg_add_section(msg, ST_NONEXT, NULL);
	
	ret = fsmsg_send(info->sd, msg, DCP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, info->sd);

	fsmsg_free(msg);
	free(path);
}

////////////////////////XXX XXX XXX XXX XXX

struct node *dir_init() {
	struct node *root;
	root = calloc(1, sizeof(struct node));
	root->name = "/";
	return root;
}

struct node *add_node(const char *path) {
	struct node *n, *p;
	n = calloc(1, sizeof(struct node));
	n->name = strdup(strrchr(path, '/')+1);
	
	p = dir_find_node(path, 0);
	//XXX bad
	add_child(p, n);
	return n;
}

void add_child(struct node *p, struct node *c) {
	struct node *i;
	
	i = p->children;
	if (!i) {
		p->children = c;
		return;
	}

	while(i->next) i=i->next;
	i->next = c;
}

struct node *dir_find_node(const char *pathc, int exact) {
	char *level, *path;
	struct node *n, *p;
	n = NULL;

	path = strdup(pathc);
	level = strtok(path, "/");
	if (!level) {
		free(path);
		return dirroot;
	}
	p = dirroot;

	while(level && level[0] != '\0') {
		fprintf(stderr, "\nlevel: %s, p->name: %s -- ", level, p->name);
		for(n=p->children;n!=NULL;n=n->next) {
			fprintf(stderr, "%s ", n->name);
			if (!strcmp(n->name, level)) {
				p=n;
				break;
			}
		}
		if (!n) break;

		level = strtok(NULL, "/");
	}

	free(path);
	return exact ? n : p;
}
