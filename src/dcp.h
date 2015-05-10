#ifndef DCP_H
#define DCP_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

enum dcp_msgtype {
	DCP_HELLO = 1,
	DCP_HELLO_RESPONSE = 2,
	DCP_LOCK = 3,
	DCP_CREATE = 4,
	DCP_READ = 5,
	DCP_UPDATE = 6,
	DCP_DELETE = 7,
	DCP_SEARCH = 8,
	DCP_INVALIDATE = 9,
	DCP_REPLICA_UPDATE = 10,
	DCP_REPLICA_STATUS = 11,
	DCP_GET_REPLICAS = 12,
	DCP_DISCONNECT = 13,
	DCP_FILEINFO = 14,
	DCP_ERROR = 16
};

struct fileserv_info {
	uint64_t id;
	int sd;
	char *name;
	int max_capacity;
	int disk_usage;
	int file_count;
};

int dcp_open(struct addrinfo *fsrv_ai, uint64_t *server_id, int32_t capacity, int32_t usage, int32_t filecnt);
int dcp_accept(struct fileserv_info *info);
struct fsmsg* dcp_create_msg(uint64_t tid, uint64_t filesytem_id, uint64_t server_id, enum dcp_msgtype msg_type);
int dcp_validate_sections(struct fsmsg *msg);
int dcp_check_response(struct fsmsg *msg, uint64_t tid, uint64_t sid, uint64_t fsid, enum dcp_msgtype request_type);
void dcp_send_error(int sd, uint64_t tid, uint64_t sid, int errorn, char *errstr);

//DCP_H
#endif
