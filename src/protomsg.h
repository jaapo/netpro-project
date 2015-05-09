#ifndef PROTOMSG_H
#define PROTOMSG_H

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define TYPEFILE 1
#define TYPEDIR 2

#define FAPPORT "1234"
#define DCPPORT "1235"
#define FCTPCTRLPORT "1236"
#define FCTPDATAPORT "1237"

#define HTONSTHIS(a) do{(a)=htons(a);}while(0)
#define HTONLTHIS(a) do{(a)=htonl(a);}while(0)
#define HTONLLTHIS(a) do{(a)=htonll(a);}while(0)

#define NTOHSTHIS(a) do{(a)=ntohs(a);}while(0)
#define NTOHLTHIS(a) do{(a)=ntohl(a);}while(0)
#define NTOHLLTHIS(a) do{(a)=ntohll(a);}while(0)

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


int fsmsg_to_buffer(struct fsmsg *msg, char **buffer, enum fsmsg_protocol protocol);
struct fsmsg* fsmsg_from_buffer(char *buffer, int len, enum fsmsg_protocol protocol);
struct fsmsg* fsmsg_from_socket(int sd, enum fsmsg_protocol protocol);
struct fsmsg* fsmsg_read(int sockd);
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

