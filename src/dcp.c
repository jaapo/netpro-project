#include "dcp.h"
#include "protomsg.h"
#include "util.h"

#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

extern uint64_t fsid;

int dcp_open(struct addrinfo *ai, uint64_t *server_id) {
	fprintf(stderr, "dcp_open() not implemented yet\n");
	return -1;
}

int dcp_accept(struct fileserv_info *info) {
	struct fsmsg *msg, *respmsg;
	char *buffer;
	int len, ret;

	msg = fsmsg_from_socket(info->sd, DCP);
	if (!msg) return -1;
	if (msg->msg_type != DCP_HELLO) return -2;
	if (dcp_validate_sections(msg)<0) {
		fsmsg_free(msg);
		return -1;
	}

	info->name = strndup(msg->sections[0]->data.string.data, msg->sections[0]->data.string.length);
	info->max_capacity = msg->sections[1]->data.integer;
	info->disk_usage = msg->sections[2]->data.integer;
	info->file_count = msg->sections[3]->data.integer;

	respmsg = dcp_create_msg(msg->tid, info->id, fsid, DCP_HELLO_RESPONSE);

	union section_data data;

	//refresh bit XXX
	data.integer = 1;
	fsmsg_add_section(respmsg, ST_INTEGER, &data);
	fsmsg_add_section(respmsg, ST_NONEXT, NULL);
	len = fsmsg_to_buffer(respmsg, &buffer, DCP);

	ret = write(info->sd, buffer, len);
	syscallerr(ret, "%s: write(%d, %p, %d) failed",__func__, info->sd, buffer, len);

	free(buffer);
	fsmsg_free(msg);
	fsmsg_free(respmsg);

	return 0;
}

struct fsmsg* dcp_create_msg(uint64_t tid, uint64_t server_id, uint64_t filesystem_id, enum dcp_msgtype msg_type) {
	struct fsmsg *msg;
	msg = fsmsg_create(DCP);
	msg->tid = tid;
	msg->ids[0] = server_id;
	msg->ids[1] = filesystem_id;
	
	msg->msg_type = (uint16_t) msg_type;

	return msg;
}

int dcp_validate_sections(struct fsmsg* msg) {
	struct msg_section **s;
	int i = 0;

	s = msg->sections;

	switch ((enum dcp_msgtype) msg->msg_type) {
		case DCP_HELLO:
			TESTST(ST_STRING);
			TESTST(ST_INTEGER);
			TESTST(ST_INTEGER);
			TESTST(ST_INTEGER);
			break;
		case DCP_HELLO_RESPONSE:
			TESTST(ST_INTEGER);
			break;
		case DCP_LOCK:
			break;
		case DCP_CREATE:
			break;
		case DCP_READ:
			break;
		case DCP_UPDATE:
			break;
		case DCP_DELETE:
			break;
		case DCP_SEARCH:
			break;
		case DCP_INVALIDATE:
			break;
		case DCP_REPLICA_UPDATE:
			break;
		case DCP_REPLICA_STATUS:
			break;
		case DCP_GET_REPLICAS:
			break;
		case DCP_DISCONNECT:
			break;
		case DCP_FILEINFO:
			break;
		case DCP_ERROR:
			break;
	}

	//ST_NONEXT always last
	TESTST(ST_NONEXT);
	return 0;
}
