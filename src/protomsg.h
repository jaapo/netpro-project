#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

enum section_type {
	nonext = 0,
	integer = 1,
	string = 2,
	binary = 3,
	fileinfo = 4
};

enum fsmsg_protocol {
	FAP, DCP, FCTP
};

struct raw_sect {
	uint32_t length;
	char *data;
};

struct fileinfo_sect {
	uint8_t type;
	uint32_t pathlen;
	char *path;
	uint32_t usernamelen;
	char *username;
	uint32_t modified;
	uint32_t size;
	uint32_t replicas;
};

union section_data {
	int32_t integer;
	struct raw_sect string;
	struct raw_sect binary;
	struct fileinfo_sect fileinfo;
};

struct msg_section {
	uint16_t type;
	union section_data data;
};

struct fsmsg {
	uint64_t tid;
	uint64_t *ids;
	uint16_t msg_type;
	struct msg_section **sections;
};


int fsmsg_to_buffer(struct fsmsg *msg, char **buffer);
struct fsmsg* fsmsg_from_buffer(char *buffer, int len, enum fsmsg_protocol protocol);
void fsmsg_create(struct fsmsg **msg, enum fsmsg_protocol protocol);
int fsmsg_add_section(struct fsmsg *msg, uint16_t type, union section_data data);
void fsmsg_free(struct fsmsg *msg);

uint64_t htonll(uint64_t host64);
uint64_t ntohll(uint64_t net64);
