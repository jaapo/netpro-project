#include "dcp.h"
#include <stdint.h>

void go_daemon();
void do_dcp_server();
void serve_fileserver(struct fileserv_info *info);

int register_fsrv(int fsrvsd, uint64_t sid);
uint64_t nextsid();
