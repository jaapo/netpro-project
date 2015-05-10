#include "fscli.h"
#include "fap.h"
#include "util.h"

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
uint64_t fsid;
uint64_t sid;

static int32_t dataport;

static char *servername;

static char *cwd;

#define ERRFAIL(x,e)do {if((x)!=e)exit(1);} while(0)

int main(int argc, char* argv[], char* envp[]) {
	add_config_param("file_server", &server);
	add_config_param("log_file", &logpath);
	read_config(CONF_FILE);

	logfile = logopen(logpath);
	print_hello();

	serv_ai = get_server_address(server, FAPPORT, logfile);
	start_connect();

	printf("connected to %s\n", servername);
	cwd = "/";
	prompt_loop();
	return 0;
}

void print_hello() {
	printf("DFS CLIENT 0.0\n");
	printf("==============\n");
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
		
		fapsd = fap_open(ai, &cid, &servername, &dataport);
		if (fapsd >= 0) break;
		printf("connection failed\n");
		logwrite(logfile, "connection failed");

		ai = ai->ai_next;
	}

	syscallerr(fapsd, "fap-connection failed %s", server);
}

#define CHECKCMD_ARGS(cmdstr, fun) \
	if (!strncmp(cmdstr " ", command, MIN(len, sizeof(cmdstr " ") - 1))) {\
		fun(command + sizeof(cmdstr " ") - 1, len - sizeof(cmdstr " ") - 1);\
	} else

#define CHECKCMD_NOARGS(cmdstr, fun) \
	if (!strncmp(cmdstr "\n", command, MIN(len, sizeof(cmdstr "\n")-1))) {\
		fun(NULL, 0);\
	} else


void prompt_loop() {
	char command[MAX_LINE];
	int len;

	for (;;) {
		printf("> ");
		fflush(stdout);
		len = readline(command, STDIN_FILENO, MAX_LINE);
		if (len == 0) client_quit();

		CHECKCMD_ARGS("cd", change_dir)
		CHECKCMD_NOARGS("pwd", working_dir)
		CHECKCMD_ARGS("ls", list_dir)
		CHECKCMD_NOARGS("ls", list_dir)
		CHECKCMD_ARGS("ls", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("cat", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("edit", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("rm", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("rmdir", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("mkdir", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("cp", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("find", NOT_IMPLEMENTED)
		CHECKCMD_NOARGS("quit", client_quit)
		printf("unknown command\n");
	}
}

void client_quit() {
	uint64_t tid;
	int ret;
	tid = fap_client_quit(fapsd, cid);
	ret = fap_client_wait_ok(fapsd, tid);
	if (ret < 0) {
		fprintf(stderr, "%s: error receiving ok-message\n", __func__);
	}
	printf("bye\n");
	exit(0);
}

void change_dir(char *args, int arglen) {
	char *dir;
	int ret, i;
	struct fileinfo_sect *files;

	ret = fap_list(fapsd, cid, 1, cwd, &files);

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

	ret = fap_list(fapsd, cid, 1, cwd, &files);

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
