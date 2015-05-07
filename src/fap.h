#include "protomsg.h"
#include <netdb.h>

enum fap_type {
	FAP_COMMAND = 5,
	FAP_HELLO = 1,
	FAP_QUIT = 3
};

uint64_t nexttid();
void fap_init_server();

int fap_open(const struct addrinfo *serv_ai, uint64_t *cid);
int fap_list(uint64_t cid, int recurse, char *current_dir, struct fileinfo_sect **files);
struct fsmsg* fap_create_msg(uint64_t tid, uint64_t server_id, uint64_t client_id, uint64_t filesystem_id, enum fap_type msg_type);
