#include "protomsg.h"
#include "fap.h"
#include "util.h"
#include "netutil.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

extern uint64_t fsid;
extern uint64_t sid;

void fap_send_error(int sd, uint64_t tid, uint64_t client_id, int errorn, char *errstr) {
	struct fsmsg *msg;
	union section_data data;
	int ret;
	
	msg = fap_create_msg(tid, sid, client_id, fsid, FAP_ERROR);

	data.integer = errorn;
	fsmsg_add_section(msg, ST_INTEGER, &data);

	data.string.length = strlen(errstr);
	data.string.data = errstr;
	fsmsg_add_section(msg, ST_STRING, &data);

	fsmsg_add_section(msg, ST_NONEXT, NULL);

	ret = fsmsg_send(sd, msg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, sd);
	
	fsmsg_free(msg);
}

int fap_open(const struct addrinfo *serv_ai, uint64_t *cid, char **servername, int *datasd) {
	int ret, sd;
	sd = socket(serv_ai->ai_family, serv_ai->ai_socktype, 0);
	syscallerr(sd, "socket");
	NO_INTR(ret = connect(sd, serv_ai->ai_addr, serv_ai->ai_addrlen));

	if (ret<0) {
		close(sd);
		return ret;
	}

	struct fsmsg *msg;
	uint64_t tid = nexttid();
	msg = fap_create_msg(tid, 0, 0, 0, FAP_HELLO);

	union section_data data;
	data.string.data = malloc(HOST_NAME_MAX);
	ret = gethostname(data.string.data, HOST_NAME_MAX);
	syscallerr(ret, "error getting name of local host with gethostbyname()");
	data.string.length = strlen(data.string.data);
	fsmsg_add_section(msg, ST_STRING, &data);
	free(data.string.data);

	data.string.data = getlogin();
	data.string.length = strlen(data.string.data);
	fsmsg_add_section(msg, ST_STRING, &data);

	fsmsg_add_section(msg, ST_NONEXT, NULL);

	ret = fsmsg_send(sd, msg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, sd);
	fsmsg_free(msg);

	msg = fsmsg_from_socket(sd, FAP);
	ret = fap_check_response(msg, tid, 0, 0, 0, FAP_HELLO);
	if (ret < 0) {
		fprintf(stderr, "invalid response to FAP_HELLO \n");
		fsmsg_free(msg);
		goto fail;
	}
	
	sid = msg->ids[0];
	*cid = msg->ids[1];
	fsid = msg->ids[2];

	ret = fap_validate_sections(msg);
	if (ret<0) {
		fap_send_error(sd, tid, *cid, ERR_MSG, "");
		fsmsg_free(msg);
		goto fail;
	}

	*servername = SECSDUP(msg, 0);
	int dataport = SECI(msg, 1);
	fsmsg_free(msg);
	
	switch (serv_ai->ai_family) {
		case AF_INET:
			((struct sockaddr_in*)serv_ai->ai_addr)->sin_port = htons(dataport);
			break;
		case AF_INET6:
			((struct sockaddr_in6*)serv_ai->ai_addr)->sin6_port = htons(dataport);
			break;
		default:
			fprintf(stderr, "unknown protocol, can't set dataport\n");
			goto fail;
	}

	*datasd = socket(serv_ai->ai_family, serv_ai->ai_socktype, 0);
	syscallerr(*datasd, "socket() failed when tried to create datasocket");
	NO_INTR(ret = connect(*datasd, serv_ai->ai_addr, serv_ai->ai_addrlen));
	if (ret < 0) {
		fprintf(stderr, "connect() failed to dataport\n");
		goto fail;
	}

	return sd;

fail:
	close(sd);
	if (*datasd != 0)
		close(*datasd);
	return -1;
}
	
uint64_t fap_client_quit(int sd, uint64_t cid) {
	struct fsmsg *msg;
	uint64_t tid = nexttid();
	int ret;

	msg = fap_create_msg(tid, sid, cid, fsid, FAP_QUIT);
	fsmsg_add_section(msg, ST_NONEXT, NULL);

	ret = fsmsg_send(sd, msg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, sd);

	fsmsg_free(msg);
	return tid;
}

int fap_client_wait_ok(int sd, uint64_t tid) {
	struct fsmsg *msg;
	int ret;

	msg = fsmsg_from_socket(sd, FAP);
	ret = fap_check_response(msg, tid, sid, 0, fsid, FAP_QUIT);
	if (ret < 0) {
		fprintf(stderr, "invalid message, error: %d\n", ret);
		return -1;
	}
	if ((enum fap_responses) SECI(msg, 0) != FAP_OK) {
		fprintf(stderr, "expected OK-response(%d)\n", FAP_OK);
		return -1;
	}

	fsmsg_free(msg);

	return 0;
}

int fap_list(int sd, uint64_t cid, int recurse, char *current_dir, struct fileinfo_sect **files) {
	struct fsmsg *msg;
	uint64_t tid = nexttid();
	int ret;

	msg = fap_create_msg(tid, sid, cid, fsid, FAP_COMMAND);
	
	union section_data data;

	data.integer = (int) FAP_CMD_LIST;
	fsmsg_add_section(msg, ST_INTEGER, &data);

	data.integer = 1;
	fsmsg_add_section(msg, ST_INTEGER, &data);

	data.string.length = strlen(current_dir);
	data.string.data = current_dir;
	fsmsg_add_section(msg, ST_STRING, &data);
	fsmsg_add_section(msg, ST_NONEXT, NULL);

	ret = fsmsg_send(sd, msg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, sd);
	
	fsmsg_free(msg);

	msg = fsmsg_from_socket(sd, FAP);
	ret = fap_check_response(msg, tid, sid, cid, fsid, FAP_COMMAND);
	if (ret < 0) {
		fprintf(stderr, "invalid response, error: %d\n", ret);
		return ret;
	}
	ret = fap_validate_sections(msg);
	if (ret < 0) {
		fprintf(stderr, "invalid response sections, error: %d\n", ret);
		return ret;
	}

	int count = SECI(msg, 1);
	*files = calloc(count, sizeof(struct fileinfo_sect));
	struct fileinfo_sect *tmp = *files;
	for(int i=0;i<count;i++) {
		fsmsg_fileinfo_copy(&tmp[i], &SECF(msg, i+2));
	}
	fsmsg_free(msg);
	return count;
}

int fap_create(int sd, uint64_t cid, char *filename) {
	struct fsmsg *msg;
	uint64_t tid = nexttid();
	int ret;

	msg = fap_create_msg(tid, sid, cid, fsid, FAP_COMMAND);
	
	union section_data data;

	data.integer = (int) FAP_CMD_CREATE;
	fsmsg_add_section(msg, ST_INTEGER, &data);

	data.string.length = strlen(filename);
	data.string.data = filename;
	fsmsg_add_section(msg, ST_STRING, &data);

	//file type
	data.integer = 1;
	fsmsg_add_section(msg, ST_INTEGER, &data);
	fsmsg_add_section(msg, ST_NONEXT, NULL);

	ret = fsmsg_send(sd, msg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, sd);
	
	fsmsg_free(msg);

	msg = fsmsg_from_socket(sd, FAP);
	ret = fap_check_response(msg, tid, sid, cid, fsid, FAP_COMMAND);
	if (ret < 0) {
		fprintf(stderr, "invalid response, error: %d\n", ret);
		return ret;
	}
	ret = fap_validate_sections(msg);
	if (ret < 0) {
		fprintf(stderr, "invalid response sections, error: %d\n", ret);
		return ret;
	}
	
	if (SECI(msg, 1) != FAP_OK) {
		return -1;
	}
	return 0;
}

int fap_write(int sd, int datasd, uint64_t cid, char *filename, char* data, int datalen) {
	struct fsmsg *msg;
	uint64_t tid = nexttid();
	int ret;

	msg = fap_create_msg(tid, sid, cid, fsid, FAP_COMMAND);
	
	union section_data sdata;

	sdata.integer = (int) FAP_CMD_WRITE;
	fsmsg_add_section(msg, ST_INTEGER, &sdata);

	sdata.string.length = strlen(filename);
	sdata.string.data = filename;
	fsmsg_add_section(msg, ST_STRING, &sdata);

	//byte count
	sdata.integer = datalen;
	fsmsg_add_section(msg, ST_INTEGER, &sdata);
	fsmsg_add_section(msg, ST_NONEXT, NULL);

	ret = fsmsg_send(sd, msg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, sd);
	
	fsmsg_free(msg);

	NO_INTR(ret = write(datasd, data, datalen));
	if (ret < 0) {
		fprintf(stderr, "%s: write of %d bytes to datasd(%d) failed\n", __func__, datalen, datasd);
		return -1;
	}

	msg = fsmsg_from_socket(sd, FAP);
	ret = fap_check_response(msg, tid, sid, cid, fsid, FAP_COMMAND);
	if (ret < 0) {
		fprintf(stderr, "invalid response, error: %d\n", ret);
		return ret;
	}
	ret = fap_validate_sections(msg);
	if (ret < 0) {
		fprintf(stderr, "invalid response sections, error: %d\n", ret);
		return ret;
	}
	
	if (SECI(msg, 0) != FAP_OK) {
		return -1;
	}
	return 0;
}

int fap_accept(struct client_info *info) {
	struct fsmsg *msg, *respmsg;
	int ret, tmpsd;

	msg = fsmsg_from_socket(info->sd, FAP);
	ret = fap_check_response(msg, 0, 0, 0, 0, 0);
	if (ret < 0) return -1;
	if (msg->msg_type != FAP_HELLO) return -1;
	if (fap_validate_sections(msg)<0) {
		fsmsg_free(msg);
		return -1;
	}

	info->lasttid = msg->tid;

	info->host = SECSDUP(msg, 0);
	info->user = SECSDUP(msg, 1);

	respmsg = fap_create_msg(msg->tid, sid, info->id, fsid, FAP_HELLO_RESPONSE);

	union section_data data;
	
	data.string.data = malloc(HOST_NAME_MAX);
	ret = gethostname(data.string.data, HOST_NAME_MAX);
	syscallerr(ret, "error getting name of local host with gethostbyname()");
	data.string.length = strlen(data.string.data);
	fsmsg_add_section(respmsg, ST_STRING, &data);

	data.integer = info->dataport;
	fsmsg_add_section(respmsg, ST_INTEGER, &data);
	fsmsg_add_section(respmsg, ST_NONEXT, NULL);
	
	ret = fsmsg_send(info->sd, respmsg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, info->sd);
	
	fsmsg_free(msg);
	fsmsg_free(respmsg);

	//XXX no ipv6
	tmpsd = start_listen(info->dataport);

	NO_INTR(ret = accept(tmpsd, NULL, NULL));
	syscallerr(ret, "listen() on datasocket");
	info->datasd = ret;

	close(tmpsd);
	
	return 0;
}

void fap_send_ok(int sd, uint64_t tid, uint64_t client_id) {
	struct fsmsg *msg;
	union section_data data;
	int ret;
	
	msg = fap_create_msg(tid, sid, client_id, fsid, FAP_RESPONSE);
	data.integer = FAP_OK;
	fsmsg_add_section(msg, ST_INTEGER, &data);
	fsmsg_add_section(msg, ST_NONEXT, NULL);
	
	ret = fsmsg_send(sd, msg, FAP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, sd);
	
	fsmsg_free(msg);
}

struct fsmsg *fap_create_msg(uint64_t tid, uint64_t server_id, uint64_t client_id, uint64_t filesystem_id, enum fap_type msg_type) {
	struct fsmsg *msg;
	msg = fsmsg_create(FAP);
	msg->tid = tid;
	msg->ids[0] = server_id;
	msg->ids[1] = client_id;
	msg->ids[2] = filesystem_id;

	msg->msg_type = (uint16_t) msg_type;

	return msg;
}

int fap_validate_sections(struct fsmsg* msg) {
	struct msg_section **s;
	int32_t cmd;
	int count, resp, i = 0;

	s = msg->sections;

	switch ((enum fap_type) msg->msg_type) {
		case FAP_HELLO:
			TESTST(ST_STRING);
			TESTST(ST_STRING);
			break;
		case FAP_HELLO_RESPONSE:
			TESTST(ST_STRING);
			TESTST(ST_INTEGER);
			break;
		case FAP_QUIT:
			break;
		case FAP_COMMAND:
			TESTST(ST_INTEGER);
			cmd = s[0]->data.integer;
			switch ((enum fap_commands) cmd) {
				case FAP_CMD_CREATE:
					TESTST(ST_STRING);
					TESTST(ST_INTEGER);
					break;
				case FAP_CMD_OPEN:
				case FAP_CMD_CLOSE:
				case FAP_CMD_STAT:
					TESTST(ST_STRING);
					break;
				case FAP_CMD_READ:
					TESTST(ST_STRING);
					TESTST(ST_INTEGER);
					TESTST(ST_INTEGER);
					break;
				case FAP_CMD_WRITE:
					TESTST(ST_STRING);
					TESTST(ST_INTEGER);
					break;
				case FAP_CMD_DELETE:
					TESTST(ST_STRING);
					TESTST(ST_INTEGER);
					break;
				case FAP_CMD_COPY:
					TESTST(ST_STRING);
					TESTST(ST_STRING);
					TESTST(ST_INTEGER);
					break;
				case FAP_CMD_FIND:
					TESTST(ST_STRING);
					TESTST(ST_STRING);
					break;
				case FAP_CMD_LIST:
					TESTST(ST_INTEGER);
					TESTST(ST_STRING);
					break;
				default:
					DEBUGPRINT("unexpected command type (%d)", cmd);
					return -1;
			}
			break;
		case FAP_RESPONSE:
			TESTST(ST_INTEGER);
			resp = s[i-1]->data.integer;
			switch ((enum fap_responses) resp) {
				case FAP_OK:
					break;
				case FAP_FILEINFO:
					TESTST(ST_INTEGER);
					count = s[i-1]->data.integer;
					for (int j=0;j<count;j++) {
						TESTST(ST_FILEINFO);
					}
					break;
				case FAP_DATAOUT:
					TESTST(ST_INTEGER);
					break;
				default:
					DEBUGPRINT("unexpected response type (%d)", resp);
					return -1;
			}
			break;
		case FAP_ERROR:
			TESTST(ST_INTEGER);
			TESTST(ST_STRING);
			break;
		default:
			DEBUGPRINT("unexpected message type (%d)", msg->msg_type);
			return -1;
	}
	//always ST_NONEXT as last
	TESTST(ST_NONEXT);

	return 0;
}

#define EXPECTID(e,id,v) do{if(e&&id!=e) {DEBUGPRINT("unexpected id (" #e "). %lu != %lu", id, e);return v;}}while(0)
#define EXPECTTYPE(et) do{if(t!=et) return -7;}while(0)
int fap_check_response(struct fsmsg *msg, uint64_t tid, uint64_t sid, uint64_t cid, uint64_t fsid, enum fap_type request_type) {
	if (!msg) return -1;
	EXPECTID(tid, msg->tid, -2);
	EXPECTID(sid, msg->ids[0], -3);
	EXPECTID(cid, msg->ids[1], -4);
	EXPECTID(fsid, msg->ids[2], -5);

	enum fap_type t;
	t = msg->msg_type;
	if (t == FAP_ERROR) {
		fprintf(stderr, "received error from server: %.*s\n", SECS(msg, 1).length, SECS(msg, 1).data);
		return -6;
	}

	if (request_type != 0) {
		switch (request_type) {
			case FAP_HELLO:
				EXPECTTYPE(FAP_HELLO_RESPONSE);
				break;
			case FAP_QUIT:
			case FAP_COMMAND:
				EXPECTTYPE(FAP_RESPONSE);
				break;
			default:
				fprintf(stderr, "%s: type %d not implemented\n", __func__, request_type);
		}
	}

	return 0;
}
