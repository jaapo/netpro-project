#ifndef PROTOMSG_H
#define PROTOMSG_H

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#define TYPEFILE 1
#define TYPEDIR 2

#define FAPPORT 1234
#define FAPPORTSTR "1234"
#define DCPPORT 1235
#define DCPPORTSTR "1235"
#define FCTPPORT 1236
#define FCTPPORTSTR "1236"

#define FAP_DATAPORT_BASE 40100

#define HTONSTHIS(a) do{(a)=htons(a);}while(0)
#define HTONLTHIS(a) do{(a)=htonl(a);}while(0)
#define HTONLLTHIS(a) do{(a)=htonll(a);}while(0)

#define NTOHSTHIS(a) do{(a)=ntohs(a);}while(0)
#define NTOHLTHIS(a) do{(a)=ntohl(a);}while(0)
#define NTOHLLTHIS(a) do{(a)=ntohll(a);}while(0)

//macro for easy section access
#define SECI(m,i) m->sections[i]->data.integer
#define SECS(m,i) m->sections[i]->data.string
#define SECB(m,i) m->sections[i]->data.binary
#define SECF(m,i) m->sections[i]->data.fileinfo

#define SECSDUP(m,i) strndup(m->sections[i]->data.string.data, m->sections[i]->data.string.length)

//macro to test section type, ST_NONEXT is not in list
//used in {fap,dcp,fctp}.c validation functions
#define TESTST(t)do{\
	if (!((s[i] == NULL && t == ST_NONEXT) || s[i]->type == t)) {\
		return -1;\
	}\
	i++;\
}while(0)

enum section_type {
	ST_NONEXT = 0,
	ST_INTEGER = 1,
	ST_STRING = 2,
	ST_BINARY = 3,
	ST_FILEINFO = 4
};

enum fsmsg_protocol {
	FAP, DCP, FCTP
};

enum fsmsg_error {
	ERR_FILENOTFOUND = 1,
	ERR_EXISTS = 2,
	ERR_LOCK = 3,
	ERR_SYNTAX = 4,
	ERR_FSERV_UNAVAILABLE = 5,
	ERR_DSERV_UNAVAILABLE = 6,
	ERR_REPLICAS = 7,
	ERR_ABORT = 8,
	ERR_MSG = 9,
	ERR_UNKNOWN = 10
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


uint64_t nexttid();
int fsmsg_to_buffer(struct fsmsg *msg, char **buffer, enum fsmsg_protocol protocol);
struct fsmsg* fsmsg_from_socket(int sd, enum fsmsg_protocol protocol);
int fsmsg_send(int sd, struct fsmsg* msg, enum fsmsg_protocol protocol);

struct fsmsg* fsmsg_create(enum fsmsg_protocol protocol);
void fsmsg_add_section(struct fsmsg *msg, uint16_t type, union section_data *data);
void fsmsg_free_section(struct msg_section *s);
void fsmsg_free(struct fsmsg *msg);
void fsmsg_fileinfo_copy(struct fileinfo_sect* dst, struct fileinfo_sect* src);
struct msg_section *fsmsg_section_copy(struct msg_section *s);

int bufferadd(char **buffer, int *buflen, char **ptr, const void *data, int datalen);
uint64_t htonll(uint64_t host64);
uint64_t ntohll(uint64_t net64);


//PROTOMSG_H
#endif

