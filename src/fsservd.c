#include "protomsg.h"
#include "fap.h"
#include "dcp.h"
#include "fsservd.h"
#include "util.h"

#include <unistd.h>
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
static uint64_t sid;
static uint64_t lastcid = 1;
static int clicnt = 0;

static struct client_info clients[MAX_CLIENTS];

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
	int ret, one = 1;
	struct sockaddr_in bindaddr;
	socklen_t bindaddrlen = sizeof(bindaddr);

	listensd = socket(AF_INET, SOCK_STREAM, 0);
	syscallerr(listensd, "%s: socket() failed", __func__);

	ret = setsockopt(listensd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	syscallerr(ret, "%s: setsockopt() failed", __func__);


	//TODO: use getaddrinfo for ipv6 support
	memset(&bindaddr, '\0', sizeof(bindaddr));
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindaddr.sin_port = htons(atoi(FAPPORT));

	ret = bind(listensd, (struct sockaddr *) &bindaddr, bindaddrlen);
	syscallerr(ret, "%s: socket() failed", __func__);

	ret = listen(listensd, 5);
	syscallerr(ret, "%s: listen() failed", __func__);
}

//TODO: threads
void do_fap_server() {
	int clisd, ret;
	struct client_info *clinfo;
	uint64_t client_id = nextcid();
	struct sockaddr *cliaddr = NULL;
	socklen_t cliaddrlen;

	fap_init_server();

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

		ret = fap_accept(clisd, client_id);
		if (ret < 0) {
			fprintf(stderr, "error accepting client");
			close(clisd);
			continue;
		}

		serve_client(clinfo);
	}
}

void serve_client(struct client_info *info) {
	struct fsmsg *msg;
	uint16_t cmd;

	for(;;) {
		msg = fsmsg_from_socket(info->sd, FAP);

		DEBUGPRINT("received message %hd from client %ld, socket %d", msg->msg_type, info->id, info->sd);

		switch ((enum fap_type) msg->msg_type) {
			case FAP_QUIT:
				DEBUGPRINT("client %ld quit", info->id);
				fap_send_ok(info->sd, msg->tid, info->id);
				close(info->sd);
				info->id = 0;
				info->sd = 0;
				
				break;
			case FAP_COMMAND:
				if (msg->sections[0]->type != integer) {
					DEBUGPRINT("%s", "invalid message. expected integer section in FAP_COMMAND.");
					goto breakfor;
				}
				cmd = msg->sections[0]->data.integer;
				switch ((enum fap_commands) cmd) {
					case FAP_CMD_CREATE:
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
breakfor:
	DEBUGPRINT("%s", "ended up to breakfor");

}

int register_client(int clisd, uint64_t cid) {
	int i;
	if (cid == 0 || clicnt > MAX_CLIENTS) return -1;
	for (i=0;i<MAX_CLIENTS;i++) {
		if (clients[i].id == 0) {
			clients[i].id = cid;
			clients[i].sd = clisd;
			clicnt++;
			break;
		}
	}
	return i;
}

uint64_t nextcid() {
	return ++lastcid;
}
