#include "protomsg.h"
#include "fap.h"
#include "util.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

static uint64_t fsid;
static uint64_t sid;

uint64_t nexttid() {
	return random();
}

void fap_init_server() {
	fsid = 1;
	sid = 1;
}

int fap_open(const struct addrinfo *serv_ai, uint64_t *cid) {
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
	fsmsg_add_section(msg, 0, NULL);

	char *buffer;
	len = fsmsg_to_buffer(msg, &buffer, FAP);
	ret = write(sd, buffer, len);
	syscallerr(ret, "%s:network error: write() failed", __func__);
	fsmsg_free(msg);


	msg = fsmsg_from_socket(sd, FAP);
	if (!msg) fprintf(stderr, "fsmsg_from_socket return NULL\n");
	if (msg->tid != tid) fprintf(stderr, "received response with invalid tid\n");
	
	sid = msg->ids[1];
	*cid = msg->ids[2];
	fsid = msg->ids[3];

	return sd;
}
	
uint64_t fap_client_quit(int sd, uint64_t cid) {
	struct fsmsg *msg;
	uint64_t tid = nexttid();
	int len, ret;
	char *buffer;

	msg = fap_create_msg(tid, sid, cid, fsid, FAP_QUIT);
	fsmsg_add_section(msg, nonext, NULL);

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
	fsmsg_add_section(msg, integer, &data);

	data.string.length = strlen(current_dir);
	data.string.data = current_dir;
	fsmsg_add_section(msg, string, &data);

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
	if (sects[0]->type != integer || sects[0]->data.integer != FAP_RESPONSE) {
		return -3;
	}

	if (sects[1]->type != integer || sects[1]->data.integer != FAP_FILEINFO) {
		return -3;
	}

	struct msg_section *s = sects[2];
	int i = 0;
	while (s->type==fileinfo) {
		*files = realloc(*files, (i+1)*sizeof(struct fileinfo_sect));
		fsmsg_fileinfo_copy(files[i], &s->data.fileinfo);
		i++;
		s = sects[i+2];
	}

	if (s->type != nonext) {
		printf("unexpected sections!\n");
	}

	return i;
}

int fap_accept(int sd, uint64_t client_id) {
	struct fsmsg *msg, *respmsg;
	char *buffer;
	int len, ret;

	msg = fsmsg_from_socket(sd, FAP);
	if (!msg) return -1;
	if (msg->msg_type != FAP_HELLO) return -2;

	respmsg = fap_create_msg(msg->tid, sid, client_id, fsid, FAP_HELLO_RESPONSE);
	fsmsg_add_section(respmsg, nonext, NULL);
	len = fsmsg_to_buffer(respmsg, &buffer, FAP);

	ret = write(sd, buffer, len);
	syscallerr(ret, "%s: write(%d, %p, %d) failed",__func__, sd, buffer, len);

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
	fsmsg_add_section(msg, nonext, NULL);
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
