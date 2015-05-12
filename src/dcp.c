#include "dcp.h"
#include "protomsg.h"
#include "util.h"

#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

extern uint64_t fsid;

int dcp_open(struct addrinfo *fsrv_ai, uint64_t *server_id, int32_t capacity, int32_t usage, int32_t filecnt) {
	int ret, sd;
	sd = socket(fsrv_ai->ai_family, fsrv_ai->ai_socktype, 0);
	NO_INTR(ret = connect(sd, fsrv_ai->ai_addr, fsrv_ai->ai_addrlen));

	if (ret<0) {
		close(sd);
		return ret;
	}
	struct fsmsg *msg;
	uint64_t tid = nexttid();
	msg = dcp_create_msg(tid, 0, 0, DCP_HELLO);

	union section_data data;
	data.string.data = malloc(HOST_NAME_MAX);
	ret = gethostname(data.string.data, HOST_NAME_MAX);
	syscallerr(ret, "error getting name of local host with gethostbyname()");
	data.string.length = strlen(data.string.data);
	fsmsg_add_section(msg, ST_STRING, &data);
	free(data.string.data);

	data.integer = capacity;
	fsmsg_add_section(msg, ST_INTEGER, &data);
	data.integer = usage;
	fsmsg_add_section(msg, ST_INTEGER, &data);
	data.integer = filecnt;
	fsmsg_add_section(msg, ST_INTEGER, &data);

	fsmsg_add_section(msg, ST_NONEXT, NULL);

	ret = fsmsg_send(sd, msg, DCP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, sd);
	
	fsmsg_free(msg);

	msg = fsmsg_from_socket(sd, DCP);
	ret = dcp_check_response(msg, tid, 0, 0, DCP_HELLO);
	if (ret < 0) {
		dcp_send_error(sd, tid, *server_id, ERR_MSG, "response type or some id was wrong");
		fsmsg_free(msg);
		return -1;
	}
	
	*server_id = msg->ids[0];
	fsid = msg->ids[1];

	ret = dcp_validate_sections(msg);
	if (ret < 0) {
		fsmsg_free(msg);
		return -1;
	}

	return sd;
}

int dcp_accept(struct fileserv_info *info) {
	struct fsmsg *msg, *respmsg;
	int ret;

	msg = fsmsg_from_socket(info->sd, DCP);
	if (!msg) return -1;
	if (msg->msg_type != DCP_HELLO) return -2;
	if (dcp_validate_sections(msg)<0) {
		fsmsg_free(msg);
		return -1;
	}

	info->name = strndup(SECS(msg, 0).data, SECS(msg, 0).length);
	info->max_capacity = SECI(msg, 1);
	info->disk_usage = SECI(msg, 2);
	info->file_count = SECI(msg, 3);

	respmsg = dcp_create_msg(msg->tid, info->id, fsid, DCP_HELLO_RESPONSE);

	union section_data data;

	//refresh bit XXX
	data.integer = 1;
	fsmsg_add_section(respmsg, ST_INTEGER, &data);
	fsmsg_add_section(respmsg, ST_NONEXT, NULL);

	ret = fsmsg_send(info->sd, respmsg, DCP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, info->sd);

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
			TESTST(ST_FILEINFO);
			break;
		case DCP_READ:
			TESTST(ST_INTEGER);
			TESTST(ST_STRING);
			break;
		case DCP_UPDATE:
			TESTST(ST_FILEINFO);
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
			TESTST(ST_INTEGER);
			for (int j=0;j<s[0]->data.integer;j++) {
				TESTST(ST_STRING);
			}
			break;
		case DCP_DISCONNECT:
			break;
		case DCP_FILEINFO:
			TESTST(ST_INTEGER);
			for (int j=0;j<s[0]->data.integer;j++) {
				TESTST(ST_FILEINFO);
			}
			break;
		case DCP_ERROR:
			break;
	}

	//ST_NONEXT always last
	TESTST(ST_NONEXT);
	return 0;
}

#define EXPECTID(e,id,v) do{if(e&&id!=e) {DEBUGPRINT("unexpected id (" #e "). %lu != %lu", id, e);return v;}}while(0)
#define EXPECTTYPE(et) do{if(t!=et) return -6;}while(0)
int dcp_check_response(struct fsmsg *msg, uint64_t tid, uint64_t sid, uint64_t fsid, enum dcp_msgtype request_type) {
	if (!msg) return -1;
	EXPECTID(tid, msg->tid, -2);
	EXPECTID(sid, msg->ids[0], -3);
	EXPECTID(fsid, msg->ids[1], -4);

	if (request_type != 0) {
		enum dcp_msgtype t;
		t = msg->msg_type;
		if (t == DCP_ERROR) return -5;

		switch (request_type) {
			case DCP_HELLO:
				EXPECTTYPE(DCP_HELLO_RESPONSE);
				break;
			case DCP_READ:
				EXPECTTYPE(DCP_FILEINFO);
				break;
			case DCP_CREATE:
				EXPECTTYPE(DCP_FILEINFO);
				break;
			case DCP_UPDATE:
				EXPECTTYPE(DCP_FILEINFO);
				break;
			case DCP_GET_REPLICAS:
				EXPECTTYPE(DCP_REPLICA_STATUS);
				break;
			default:
				fprintf(stderr, "%s: type %d not implemented\n", __func__, request_type);
		}
	}

	return 0;
}

void dcp_send_error(int sd, uint64_t tid, uint64_t sid, int errorn, char *errstr) {
	struct fsmsg *msg;
	union section_data data;
	int ret;
	
	msg = dcp_create_msg(tid, sid, fsid, DCP_ERROR);

	data.integer = errorn;
	fsmsg_add_section(msg, ST_INTEGER, &data);

	data.string.length = strlen(errstr);
	data.string.data = errstr;
	fsmsg_add_section(msg, ST_STRING, &data);

	fsmsg_add_section(msg, ST_NONEXT, NULL);

	ret = fsmsg_send(sd, msg, DCP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, sd);

	fsmsg_free(msg);
}

int dcp_send_fileinfo(struct fileserv_info *srv, struct fileinfo_sect *file) {
	struct fsmsg *msg;
	union section_data data;
	int ret;
	msg = dcp_create_msg(srv->lasttid, srv->id, fsid, DCP_FILEINFO);
	
	memset(&data, '\0', sizeof(data));
	data.integer = 1;
	fsmsg_add_section(msg, ST_INTEGER, &data);
	data.fileinfo = *file;
	fsmsg_add_section(msg, ST_FILEINFO, &data);
	fsmsg_add_section(msg, ST_NONEXT, NULL);
	
	ret = fsmsg_send(srv->sd, msg, DCP);
	fsmsg_free(msg);
	return ret;
}
