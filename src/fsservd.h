#include "fap.h"
#include <stdint.h>

void go_daemon();
void connect_dirserv();
void do_fap_server();
void serve_client(struct client_info *info);
int register_client(int clisd, uint64_t cid);
uint64_t nextcid();

