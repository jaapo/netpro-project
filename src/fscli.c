#include "fscli.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>

static const char *CONF_FILE = "conf/dfs_client.conf";
static char *server;
static char *logpath;

static int logfile;

struct addrinfo *serv_ai;

#define ERRFAIL(x,e)do {if((x)!=e)exit(1);} while(0)

int main(int argc, char* argv[], char* envp[]) {
	int ret;

	add_config_param("file_server", &server);
	add_config_param("log_file", &logpath);
	read_config(CONF_FILE);

	logfile = logopen(logpath);
	print_hello();


	start_connect();
//	show_prompt();
	return 0;
}

void print_hello() {
	printf("DFS CLIENT 0.0\n");
	printf("==============\n");
}

void get_server_address() {
	int ret;

	struct addrinfo hints;	
	ret = getaddrinfo(server, "fap", &hints, &serv_ai);
	if (ret != 0) {
		logwrite(logfile, "getaddrinfo error: %s", gai_strerror);
	}

}

void start_connect() {
	printf("connecting to %s (%s)...\n", server, servaddr);
}
