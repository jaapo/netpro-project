enum fctp_msgtype {
	FCTP_DOWNLOAD = 1,
	FCTP_OK = 2,
	FCTP_ERROR = 3
};

int fctp_get_file(char *hostname, char *path, char *dataloc);
struct fsmsg* fctp_create_msg(uint64_t tid, uint64_t server_id, uint64_t filesystem_id, enum fctp_msgtype msg_type);
int fctp_connect(char *host);
void fctp_server(char *dataloc);
void fctp_send_error(int sd, uint64_t tid, int errorn, char *errstr);
