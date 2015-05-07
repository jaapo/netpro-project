#include "fscli.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <time.h>
#include <string.h>

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

#ifndef NI_MAXSERV
#define NI_MAXSERV 32
#endif

static const char *CONF_FILE = "conf/dfs_client.conf";
static char *server;
static char *logpath;

static FILE *logfile;

struct addrinfo *serv_ai;
static int fapsd;
static uint64_t cid;

static char *cwd;

#define ERRFAIL(x,e)do {if((x)!=e)exit(1);} while(0)

int main(int argc, char* argv[], char* envp[]) {
	add_config_param("file_server", &server);
	add_config_param("log_file", &logpath);
	read_config(CONF_FILE);

	logfile = logopen(logpath);
	print_hello();

	get_server_address();
	start_connect();
	prompt_loop();
	return 0;
}

void print_hello() {
	printf("DFS CLIENT 0.0\n");
	printf("==============\n");
}

void get_server_address() {
	int ret;

	struct addrinfo hints;
	hints.ai_flags = AI_V4MAPPED;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	ret = getaddrinfo(server, "123", &hints, &serv_ai);
	if (ret != 0) {
		logwrite(logfile, "getaddrinfo error: %s", gai_strerror(ret));
	}
}

void start_connect() {
	int ret;
	struct addrinfo *ai;
	ai = serv_ai;

	while (ai) {
		char servaddr[NI_MAXHOST];
		char servport[NI_MAXSERV];
		ret = getnameinfo(ai->ai_addr, ai->ai_addrlen, servaddr, NI_MAXHOST, servport, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

		if (ret != 0) {
			logwrite(logfile, "getnameinfo error: %s", gai_strerror(ret));
			servaddr[0] = '\0';
			servport[0] = '\0';
		}
		
		printf("connecting to %s (%s:%s)...\n", server, servaddr, servport);
		logwrite(logfile, "connecting to %s (%s:%s)", server, servaddr, servport);
		
		fapsd = fap_open(ai, &cid);
		if (fapsd >= 0) break;
		printf("connection failed\n");
		logwrite(logfile, "connection failed");

		ai = ai->ai_next;
	}

	syscallerr(fapsd, "fap-connection failed %s", server);
}

#define CHECKCMD(cmdstr, fun) do{\
	if (!strncmp(cmdstr " ", command, (len < sizeof(cmdstr " ") ? len : sizeof(cmdstr " ")))) {\
		fun(command + _cmdlen + 1, len - sizeof(cmdstr " ") - 1);\
	}

#define MIN(a,b) ((a)<(b)?(a):(b))

void prompt_loop() {
	char command[MAX_LINE];
	int len;

	for (;;) {
		printf("> ");
		len = readline(command, STDIN_FILENO, MAX_LINE);
		if (len == 0) continue;

		if (!strncmp(command, "cd ", MIN(3, len))) {
			change_dir(command + 3, len-3);
		} else if (!strncmp(command, "pwd ", MIN(4, len))) {
			working_dir(command + 4, len-3);
		} else if (!strncmp(command, "ls ", MIN(3, len))) {
			list_dir(command + 3, len - 3);
		} else if (!strncmp(command, "cat ", MIN(4, len))) {

		} else if (!strncmp(command, "edit ", MIN(5, len))) {

		} else if (!strncmp(command, "rm ", MIN(3, len))) {

		} else if (!strncmp(command, "rmdir ", MIN(6, len))) {
		} else if (!strncmp(command, "mkdir ", MIN(6, len))) {
		} else if (!strncmp(command, "cp ", MIN(3, len))) {
		} else if (!strncmp(command, "find ", MIN(5, len))) {
		} else if (!strncmp(command, "quit ", MIN(5, len))) {

		} else {
			printf("unknown command\n");
		}
	}
}	

void change_dir(char *args, int arglen) {
	char *dir;
	int ret, i;
	struct fileinfo_sect *files;

	ret = fap_list(cid, 1, cwd, &files);

	if (ret < 0) {
		printf("change_dir listing error\n");
		return;
	}

	dir = strtok(args, "\n ");
	for (i=0;i<ret;i++) {
		if (files[i].type==TYPEDIR && !strncmp(files[i].path, dir, files[i].pathlen)) {
				break;
		}
	}

	if (i==ret) {
		printf("dir %s not found\n", dir);
		return;
	}

	strcpy(cwd, dir);
}

void working_dir(char *args, int arglen) {
	printf("%s\n", cwd);
}

void list_dir(char *args, int arglen) {
	int ret, i;
	struct fileinfo_sect *files;

	ret = fap_list(cid, 1, cwd, &files);

	if (ret < 0) {
		printf("change_dir listing error\n");
		return;
	}

	for(i=0;i<ret;i++) {
		char modtime[128];
		strftime(modtime, 128, "%F %T %z ", localtime((time_t*) &files[i].modified));
		char *name, filetype;
		int namelen;

		name = (char *) memrchr(files[i].path, '/', files[i].pathlen) + 1;
		namelen = files[i].pathlen - (name - files[i].path) - 1;

		filetype = files[i].type==1?'f':'d';

		// modified user size type name
		printf("%s %.*s %6u %c %.*s", modtime, files[i].usernamelen, files[i].username, files[i].size, filetype, namelen, name);
	}

	printf("%d files\n", i);

}
