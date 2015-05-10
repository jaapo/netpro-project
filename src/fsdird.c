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

#define SYSLOGPRIO (LOG_USER | LOG_ERR)

#define MAX_FSERVERS 100

static char *conffile;

static char *dataloc;
static int minreplicas;

static int listensd;

static uint64_t lastsid = 0;

uint64_t fsid;

static struct fileserv_info fileservers[MAX_FSERVERS];
static int fsrvcnt = 0;

int main(int argc, char* argv[], char* envp[]) {
	char *tmp;

	conffile = read_args(argc, argv, "/etc/dfs_dirserv.conf");

	add_config_param("data_location", &dataloc);
	add_config_param("min_replicas", &tmp);
	read_config(conffile);

	minreplicas = atoi(tmp);

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

		SYSLOG(SYSLOGPRIO, "connection from %s", fsrvinfo->name);
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

}

uint64_t nextsid() {
	return ++lastsid;
}
