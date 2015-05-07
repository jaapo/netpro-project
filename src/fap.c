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
	len = fsmsg_to_buffer(msg, &buffer);
	ret = write(sd, buffer, len);
	syscallerr(ret, "network error: fap_open write failed");
	fsmsg_free(msg);


	msg = fsmsg_from_socket(sd, FAP);
	if (!msg) printf("fsmsg_from_socket return NULL\n");
	if (msg->tid != tid) printf("received response with invalid tid\n");
	
	sid = msg->ids[1];
	*cid = msg->ids[2];
	fsid = msg->ids[3];

	return sd;
}
	
int fap_list(uint64_t cid, int recurse, char *current_dir, struct fileinfo_sect **files) {
	struct fsmsg *msg;
	uint64_t tid = nexttid();

	msg = fap_create_msg(tid, sid, cid, fsid, FAP_COMMAND);
	
	union section_data data;
	data.integer = 0;
	fsmsg_add_section(msg, integer, &data);

	return 0;
}

struct fsmsg *fap_create_msg(uint64_t tid, uint64_t server_id, uint64_t client_id, uint64_t filesystem_id, enum fap_type msg_type) {
	struct fsmsg *msg;
	msg = fsmsg_create(FAP);
	msg->ids[0] = htonll(tid);
	msg->ids[1] = htonll(server_id);
	msg->ids[2] = htonll(client_id);
	msg->ids[3] = htonll(filesystem_id);

	msg->msg_type = htons((uint16_t) msg_type);

	return msg;
}
