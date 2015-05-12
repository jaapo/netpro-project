#ifndef FAP_H
#define FAP_H

#include "protomsg.h"
#include <netdb.h>

enum fap_type {
	FAP_HELLO = 1,
	FAP_HELLO_RESPONSE = 2,
	FAP_QUIT = 3,
	FAP_COMMAND = 5,
	FAP_RESPONSE = 6,
	FAP_ERROR = 8
};

enum fap_commands {
	FAP_CMD_CREATE = 1,
	FAP_CMD_OPEN = 2,
	FAP_CMD_CLOSE = 3,
	FAP_CMD_STAT = 4,
	FAP_CMD_READ = 5,
	FAP_CMD_WRITE = 6,
	FAP_CMD_DELETE = 7,
	FAP_CMD_COPY = 8,
	FAP_CMD_FIND = 9,
	FAP_CMD_LIST = 10
};

enum fap_responses {
	FAP_OK = 1,
	FAP_FILEINFO = 2,
	FAP_DATAOUT = 3
};

struct client_info {
	uint64_t id;
	uint64_t lasttid;
	int sd;
	int datasd;
	char *user;
	char *host;
	int dataport;
};

//common functions
void fap_send_error(int sd, uint64_t tid, uint64_t client_id, int errorn, char *errstr);

//client functions
int fap_open(const struct addrinfo *serv_ai, uint64_t *cid, char **servername, int32_t *dataport);
uint64_t fap_client_quit(int sd, uint64_t cid);
int fap_client_wait_ok(int sd, uint64_t tid);
int fap_list(int sd, uint64_t cid, int recurse, char *current_dir, struct fileinfo_sect **files);
int fap_create(int sd, uint64_t cid, char *filename);
int fap_write_file(int sd, int datasd, uint64_t cid, char *filename, char* data, int datalen);
int fap_read_file(int sd, int datasd, uint64_t cid, char *filename);

//server functions
int fap_accept(struct client_info *info);
void fap_send_ok(int sd, uint64_t tid, uint64_t client_id);

//message functions
struct fsmsg* fap_create_msg(uint64_t tid, uint64_t server_id, uint64_t client_id, uint64_t filesystem_id, enum fap_type msg_type);
int fap_validate_sections(struct fsmsg* msg);
int fap_check_response(struct fsmsg *msg, uint64_t tid, uint64_t sid, uint64_t cid,uint64_t fsid, enum fap_type request_type);

//FAP_H
#endif
