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
	srandom(3);
	add_config_param("file_server", &server);
	add_config_param("log_file", &logpath);
	read_config(CONF_FILE);

	logfile = logopen(logpath);
	print_hello();

	serv_ai = get_server_address(server, FAPPORTSTR, logfile);
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
		fun(command + sizeof(cmdstr " ") - 1, len - (sizeof(cmdstr " ") - 1));\
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

		CHECKCMD_ARGS("cd", NOT_IMPLEMENTED)
		CHECKCMD_NOARGS("pwd", working_dir)
		CHECKCMD_NOARGS("ls", list_dir)
		CHECKCMD_ARGS("ls", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("cat", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("edit", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("rm", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("rmdir", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("mkdir", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("cp", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("find", NOT_IMPLEMENTED)
		CHECKCMD_ARGS("touch", new_file)
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

void working_dir(char *args, int arglen) {
	printf("%s\n", cwd);
}

void list_dir(char *args, int arglen) {
	int ret;
	struct fileinfo_sect *files = NULL;

	ret = fap_list(fapsd, cid, 1, cwd, &files);

	if (ret < 0) {
		printf("change_dir listing error\n");
		return;
	}

	for(int i=0;i<ret;i++) {
		char *name;
		int namelen;

		name = files[i].path;
		namelen = files[i].pathlen;

		// modified user size type name
		printf("%.*s ", namelen, name);
	}

	printf("%s%d files\n", ret?"\n":"", ret);
}

void new_file(char *args, int arglen) {
	int ret;
	char *filename = malloc(arglen + strlen(cwd));
	args[arglen-1] = '\0';
	filename[0] = '\0';
	strcat(filename, cwd);
	strcat(filename, args);
	ret = fap_create(fapsd, cid, filename);
	free(filename);
	if (ret < 0) {
		printf("error\n");
		return;
	}
}

