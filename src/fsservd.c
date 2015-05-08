#include "protomsg.h"
#include "fap.h"
#include "dcp.h"
#include "fsservd.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>

#define SYSLOGPRIO LOG_USER | LOG_ERR

static char *conffile;

char *dirserver;
char *dataloc;
char *maxspace;

int listensd;

struct addrinfo *dserv_ai;
int dssd;
uint64_t sid;

int main(int argc, char* argv[], char* envp[]) {
	int ret;
	read_args(argc, argv);

	add_config_param("directory_server", &dirserver);
	add_config_param("data_location", &dataloc);
	add_config_param("maximum_space", &maxspace);

	ret = read_config(conffile);
	if (ret != 3) {
		fprintf(stderr, "config file error (%s), %d parmeters found, expected 3\n", conffile, ret);
		exit(1);
	}
	
	go_daemon();
	dserv_ai = get_server_address(dirserver, DCPPORT, NULL);
	connect_dirserv();

	start_listen();
	do_fap_server();

	return 0;
}

void read_args(int argc, char* argv[]) {
	for (int i=1;i<argc-1;i++) {
		if (!strcmp(argv[i], "-conf")) {
			conffile = argv[i+1];
			return;
		}
	}
	conffile = "/etc/dfs_fileserv.conf";
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
			syslog(SYSLOGPRIO, "getnameinfo error: %s", gai_strerror(ret));
			servaddr[0] = '\0';
			servport[0] = '\0';
		}
		
		printf("connecting to directory server %s (%s:%s)...\n", dirserver, servaddr, servport);
		syslog(SYSLOGPRIO, "connecting to directory server %s (%s:%s)", dirserver, servaddr, servport);
		
		dssd = dcp_open(ai, &sid);
		if (dssd >= 0) break;
		printf("connection failed\n");
		syslog(SYSLOGPRIO, "connection failed");

		ai = ai->ai_next;
	}

	//syscallerr(dssd, "dcp-connection failed %s", dirserver);	
}

void start_listen() {
	int ret;
	struct sockaddr_in bindaddr;
	socklen_t bindaddrlen = sizeof(bindaddr);

	listensd = socket(AF_INET, SOCK_STREAM, 0);
	syscallerr(listensd, "start_listen(): socket() failed");

	//TODO: use getaddrinfo for ipv6 support
	memset(&bindaddr, '\0', sizeof(bindaddr));
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindaddr.sin_port = htons(1234);

	ret = bind(listensd, (struct sockaddr *) &bindaddr, bindaddrlen);
	syscallerr(ret, "start_listen(): socket() failed");

	ret = listen(listensd, 5);
	syscallerr(ret, "start_listen(): listen() failed");
}

//TODO: threads
void do_fap_server() {
	int clisd, ret;
	struct sockaddr *cliaddr = NULL;
	socklen_t cliaddrlen;

	fap_init_server();

	for (;;) {
		clisd = accept(listensd, cliaddr, &cliaddrlen);
		if (clisd < 0 && errno == EINTR) continue;
		syscallerr(clisd, "do_fap_server: accept() failed");

		ret = fap_accept(clisd);
	}
}
