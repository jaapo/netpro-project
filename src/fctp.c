#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include "protomsg.h"
#include "netutil.h"
#include "util.h"
#include "fctp.h"

extern uint64_t sid;
extern uint64_t fsid;

int fctp_get_file(char *hostname, char *path, char *dataloc) {
	int ret, sd;
	struct fsmsg *msg;
	union section_data data;
	memset(&data, 0, sizeof(data));
	
	sd = fctp_connect(hostname);
	msg = fctp_create_msg(nexttid(), sid, fsid, FCTP_DOWNLOAD);
	data.string.length = strlen(path);
	data.string.data = path;

	fsmsg_add_section(msg, ST_STRING, &data);
	fsmsg_add_section(msg, ST_NONEXT, NULL);
	ret = fsmsg_send(sd, msg, FCTP);
	syscallerr(ret, "%s: fsmsg_send() failed, socket=%d", __func__, sd);
	fsmsg_free(msg);
	
	msg = fsmsg_from_socket(sd, FCTP);
	close(sd);
	if (!msg) return 0;
	//XXX no section validation, or other checking yet
	if (msg->msg_type == FCTP_ERROR) {
		fsmsg_free(msg);
		return 0;
	} else if (msg->msg_type == FCTP_OK) {
		char *localpath = malloc(strlen(dataloc) + strlen(path) + 1);
		strcpy(localpath, dataloc);
		strcat(localpath, path);

		int fd;
		fd = open(localpath, O_CREAT|O_WRONLY);
		if (fd > 0) {
			write(fd, SECB(msg, 0).data, SECB(msg, 0).length);
		}

		close(fd);
		free(localpath);
		return 1;
	}

	return 0;
}

struct fsmsg* fctp_create_msg(uint64_t tid, uint64_t server_id, uint64_t filesystem_id, enum fctp_msgtype msg_type) {
	struct fsmsg *msg;
	msg = fsmsg_create(FCTP);
	msg->tid = tid;
	msg->ids[0] = server_id;
	msg->ids[1] = filesystem_id;
	
	msg->msg_type = (uint16_t) msg_type;

	return msg;
}

int fctp_connect(char *host) {
	int ret, sd;
	struct addrinfo *serv_ai, *ai;
	serv_ai = get_server_address(host, FCTPPORTSTR, NULL);
	ai = serv_ai;

	while (ai) {
		sd = socket(ai->ai_family, ai->ai_socktype, 0);
		NO_INTR(ret = connect(sd, ai->ai_addr, ai->ai_addrlen));
		if (ret > 0) return ret;

		ai = ai->ai_next;
	}

	return sd;
}
