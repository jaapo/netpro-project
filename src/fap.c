#include "protomsg.h"
#include "fap.h"
#include "util.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

extern uint64_t fsid;
extern uint64_t sid;

uint64_t nexttid() {
	return random();
}

void fap_send_error(int sd, uint64_t tid, uint64_t client_id, int errorn, char *errstr) {
	struct fsmsg *msg;
	union section_data data;
	char *buffer;
	int len, ret;
	
	msg = fap_create_msg(tid, sid, client_id, fsid, FAP_ERROR);

	data.integer = errorn;
	fsmsg_add_section(msg, ST_INTEGER, &data);

	data.string.length = strlen(errstr);
	data.string.data = errstr;
	fsmsg_add_section(msg, ST_STRING, &data);

	fsmsg_add_section(msg, ST_NONEXT, NULL);

	len = fsmsg_to_buffer(msg, &buffer, FAP);

	ret = write(sd, buffer, len);
	syscallerr(ret, "%s: write(%d, %p, %d) failed",__func__, sd, buffer, len);

	free(buffer);
	fsmsg_free(msg);
}

void fap_init_server() {
	fsid = 1;
	sid = 1;
}

int fap_open(const struct addrinfo *serv_ai, uint64_t *cid, char **servername, int32_t *dataport) {
	int ret, sd, len;
	sd = socket(serv_ai->ai_family, serv_ai->ai_socktype, 0);
	ret = connect(sd, serv_ai->ai_addr, serv_ai->ai_addrlen);

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

	char *buffer;
	len = fsmsg_to_buffer(msg, &buffer, FAP);
	ret = write(sd, buffer, len);
	syscallerr(ret, "%s:network error: write() failed", __func__);
	fsmsg_free(msg);

	msg = fsmsg_from_socket(sd, FAP);
	if (!msg) fprintf(stderr, "fsmsg_from_socket return NULL\n");
	if (msg->msg_type != FAP_HELLO_RESPONSE) fprintf(stderr, "expected FAP_HELLO_RESPONSE (%d), received %d\n", FAP_HELLO_RESPONSE, msg->msg_type);
	if (msg->tid != tid) fprintf(stderr, "received response with invalid tid\n");
	
	sid = msg->ids[1];
	*cid = msg->ids[2];
	fsid = msg->ids[3];

	ret = fap_validate_sections(msg);
	if (ret<0) {
		fap_send_error(sd, tid, *cid, ERR_MSG, "");
		fsmsg_free(msg);
		return -1;
	}

	*servername = strndup(msg->sections[0]->data.string.data, msg->sections[0]->data.string.length);
	*dataport = msg->sections[1]->data.integer;

	fsmsg_free(msg);
	return sd;
}
	
uint64_t fap_client_quit(int sd, uint64_t cid) {
	struct fsmsg *msg;
	uint64_t tid = nexttid();
	int len, ret;
	char *buffer;

	msg = fap_create_msg(tid, sid, cid, fsid, FAP_QUIT);
	fsmsg_add_section(msg, ST_NONEXT, NULL);

	len = fsmsg_to_buffer(msg, &buffer, FAP);

	ret = write(sd, buffer, len);
	syscallerr(ret, "%s: write(%d, %p, %d) failed", __func__, sd, buffer, len);

	free(buffer);
	fsmsg_free(msg);
	return tid;
}

int fap_client_wait_ok(int sd, uint64_t tid) {
	struct fsmsg *msg;

	msg = fsmsg_from_socket(sd, FAP);
	if (!msg) {
		fprintf(stderr, "fsmsg_from_socket return NULL\n");
		return -1;
	}
	if (msg->tid != tid) {
		fprintf(stderr, "received response with invalid tid\n");
		return -1;
	}
	if ((enum fap_responses) msg->msg_type != FAP_OK) {
		fprintf(stderr, "expected OK-response(%d), received %d\n", FAP_OK, (enum fap_responses) msg->msg_type);
		return -1;
	}

	fsmsg_free(msg);

	return 0;
}

int fap_list(int sd, uint64_t cid, int recurse, char *current_dir, struct fileinfo_sect **files) {
	struct fsmsg *msg;
	uint64_t tid = nexttid();
	int len, ret;
	char *buffer;

	msg = fap_create_msg(tid, sid, cid, fsid, FAP_COMMAND);
	
	union section_data data;

	data.integer = (int) FAP_CMD_LIST;
	fsmsg_add_section(msg, ST_INTEGER, &data);

	data.string.length = strlen(current_dir);
	data.string.data = current_dir;
	fsmsg_add_section(msg, ST_STRING, &data);

	len = fsmsg_to_buffer(msg, &buffer, FAP);

	ret = write(sd, buffer, len);
	syscallerr(ret, "%s: write(%d, %p, %d) failed", __func__, sd, buffer, len);

	free(buffer);
	fsmsg_free(msg);

	msg = fsmsg_from_socket(sd, FAP);

	if (tid != msg->tid) return -2;
	if (sid != msg->ids[0]) return -2;
	if (cid != msg->ids[1]) return -2;
	if (fsid != msg->ids[2]) return -2;

	struct msg_section **sects = msg->sections;
	if (sects[0]->type != ST_INTEGER || sects[0]->data.integer != FAP_RESPONSE) {
		return -3;
	}

	if (sects[1]->type != ST_INTEGER || sects[1]->data.integer != FAP_FILEINFO) {
		return -3;
	}

	struct msg_section *s = sects[2];
	int i = 0;
	while (s->type==ST_FILEINFO) {
		*files = realloc(*files, (i+1)*sizeof(struct fileinfo_sect));
		fsmsg_fileinfo_copy(files[i], &s->data.fileinfo);
		i++;
		s = sects[i+2];
	}

	if (s->type != ST_NONEXT) {
		printf("unexpected sections!\n");
	}

	return i;
}

int fap_accept(struct client_info *info) {
	struct fsmsg *msg, *respmsg;
	char *buffer;
	int len, ret;

	msg = fsmsg_from_socket(info->sd, FAP);
	if (!msg) return -1;
	if (msg->msg_type != FAP_HELLO) return -2;
	if (fap_validate_sections(msg)<0) {
		fsmsg_free(msg);
		return -1;
	}

	info->host = strndup(msg->sections[0]->data.string.data, msg->sections[0]->data.string.length);
	info->user = strndup(msg->sections[1]->data.string.data, msg->sections[1]->data.string.length);

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
	len = fsmsg_to_buffer(respmsg, &buffer, FAP);

	ret = write(info->sd, buffer, len);
	syscallerr(ret, "%s: write(%d, %p, %d) failed",__func__, info->sd, buffer, len);

	free(buffer);
	fsmsg_free(msg);
	fsmsg_free(respmsg);

	return 0;
}

void fap_send_ok(int sd, uint64_t tid, uint64_t client_id) {
	struct fsmsg *msg;
	char *buffer;
	int len, ret;
	
	msg = fap_create_msg(tid, sid, client_id, fsid, FAP_OK);
	fsmsg_add_section(msg, ST_NONEXT, NULL);
	len = fsmsg_to_buffer(msg, &buffer, FAP);

	ret = write(sd, buffer, len);
	syscallerr(ret, "%s: write(%d, %p, %d) failed",__func__, sd, buffer, len);

	free(buffer);
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
