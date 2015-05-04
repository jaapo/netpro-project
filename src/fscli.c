#include "fscli.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static const char *CONFIG = "dfs_client.conf";
static char *server;
static char *logpath;

int main(int argc, char* argv[], char* envp[]) {
//	check_args(argc, argv);

	add_config_param("file_server", &server);
	add_config_param("log_file", &logpath);
	read_config(CONFIG);

	printf("file_server: %s\n", server);
	printf("log_file: : %s\n", logpath);

//	print_hello();
//	start_connect();
//	show_prompt();
	return 0;
}

int check_args(int argc, char* argv[]) {
	return 1;
}

