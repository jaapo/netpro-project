#include "fap.h"
#include <stdint.h>

void go_daemon();
void connect_dirserv();
void do_fap_server();
void serve_client(struct client_info *info);
int register_client(int clisd, uint64_t cid);
uint64_t nextcid();

void list_directory(struct client_info *info, int recurse, char *path, int pathlen);
void create_file(struct client_info *info, char *path);
void write_file(struct client_info *info, char *path, int length);
void serve_file(struct client_info *info, char *path);
void ask_file(char *path);
