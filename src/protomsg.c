#include "protomsg.h"

#define FREEIF(x) do{if(x)free(x);}while(0)

void bufferadd(char **buffer, int *buflen, char **ptr, const void *data, int datalen) {
	int curlen = *ptr-*buffer;

	if (curlen+datalen > *buflen) {
		*buflen += datalen+64;
		*buffer = realloc(*buffer, *buflen);
		*ptr = *buffer+curlen;
	}

	memcpy(*ptr, data, datalen);
	*ptr += datalen;
}

int fsmsg_to_buffer(struct fsmsg *msg, char **buffer) {
	char *buf, *data;
	int len = 1024;

	buf = malloc(len);
	data = buf;

	uint64_t tmpuint64;
	uint32_t tmpuint32;
	uint16_t tmpuint16;
	uint8_t tmpuint8;
	int32_t tmpint32;

	//transaction id
	tmpuint64 = htonll(msg->tid);
	bufferadd(&buf, &len, &data, (void *)&tmpuint64, sizeof(tmpuint64));
	
	//other ids
	for (int i=0;(tmpuint64 = msg->ids[i]);i++) {
		bufferadd(&buf, &len, &data, (void *)&tmpuint64, sizeof(tmpuint64));
	}

	//message type
	tmpuint16 = htons(msg->msg_type);
	bufferadd(&buf, &len, &data, (void *)&tmpuint16, sizeof(tmpuint16));
	
	//sections
	struct msg_section *section;
	for (int i=0;(section = msg->sections[i]);i++) {
		switch ((enum section_type) section->type) {
			case integer:
				tmpint32 = htonl(section->data.integer);
				bufferadd(&buf, &len, &data, (void *)&tmpint32, sizeof(tmpint32));
				break;
			case string:
				tmpuint32 = htonl(section->data.string.length);
				bufferadd(&buf, &len, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				bufferadd(&buf, &len, &data, (void *)section->data.string.data, section->data.string.length);
				break;
			case binary:
				tmpuint32 = htonl(section->data.binary.length);
				bufferadd(&buf, &len, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				bufferadd(&buf, &len, &data, (void *)section->data.binary.data, section->data.binary.length);
				break;
			case fileinfo:
				tmpuint8 = section->data.fileinfo.type;
				bufferadd(&buf, &len, &data, (void *)&tmpuint8, sizeof(tmpuint8));
				
				tmpuint32 = htonl(section->data.fileinfo.pathlen);
				bufferadd(&buf, &len, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				bufferadd(&buf, &len, &data, (void *)section->data.fileinfo.path, section->data.fileinfo.pathlen);

				tmpuint32 = htonl(section->data.fileinfo.usernamelen);
				bufferadd(&buf, &len, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				bufferadd(&buf, &len, &data, (void *)section->data.fileinfo.username, section->data.fileinfo.usernamelen);
				
				tmpuint32 = htonl(section->data.fileinfo.modified);
				bufferadd(&buf, &len, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				tmpuint32 = htonl(section->data.fileinfo.size);
				bufferadd(&buf, &len, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				tmpuint32 = htonl(section->data.fileinfo.replicas);
				bufferadd(&buf, &len, &data, (void *)&tmpuint32, sizeof(tmpuint32));
			default:
				break;
		}
	}

	return len;
}

int readval(const char *buf, int len, char **ptr, int readlen, void *dst) {
	if (*ptr-buf+readlen > len) {
		return 0;
	} else {
		memcpy(dst, *ptr, readlen);
		*ptr += readlen;
		return readlen;
	}
}

#define TRY(expr) do {if((expr)==0) goto fail;}while(0)

struct fsmsg* fsmsg_from_buffer(char *buffer, int len, enum fsmsg_protocol protocol) {
	char *ptr;
	uint64_t tmpuint64;
	uint32_t tmpuint32;
	uint16_t tmpuint16;
	uint8_t tmpuint8;
	int32_t tmpint32;
	ptr = buffer;

	struct fsmsg *msg;
	fsmsg_create(&msg, protocol);

	//read ids, transaction id is first
	TRY(readval(buffer, len, &ptr, sizeof(msg->tid), &tmpuint64));
	tmpuint64 = ntohll(tmpuint64);

	int idcount;
	switch (protocol) {
		case FAP:
		case FCTP:
			idcount = 3;
			break;
		case DCP:
			idcount = 2;
			break;
	}

	msg->ids = malloc((idcount+1)*sizeof(uint64_t));
	for (int i=0;i<idcount;i++) {
		TRY(readval(buffer, len, &ptr, sizeof(msg->tid), &tmpuint64));
		tmpuint64 = ntohll(tmpuint64);
		msg->ids[i] = tmpuint64;
	}
	
	//message type
	TRY(readval(buffer, len, &ptr, sizeof(msg->msg_type), &tmpuint16));
	tmpuint16 = ntohs(tmpuint16);
	
	//sections
	uint16_t sectype;
	int sectioncount;
	struct msg_section *s;
	while(1) {
		TRY(readval(buffer, len, &ptr, sizeof(tmpuint16), &tmpuint16));
		sectype = (enum section_type) ntohs(tmpuint16);
		if (sectype == nonext) break;

		sectioncount++;
		msg->sections = realloc(msg->sections, (sectioncount+1)*sizeof(struct msg_section*));
		msg->sections[sectioncount] = NULL;
		s = malloc(sizeof(struct msg_section));
		msg->sections[sectioncount-1] = s;

		s->type = sectype;

		switch (sectype) {
			struct fileinfo_sect *fi;
			case integer:
				TRY(readval(buffer, len, &ptr, sizeof(tmpint32), &tmpint32));
				s->data.integer = ntohl(tmpint32);
				break;
			case string:
				//read length
				TRY(readval(buffer, len, &ptr, sizeof(tmpuint32), &tmpuint32));
				s->data.string.length = ntohl(tmpuint32);

				//read data
				s->data.string.data = malloc(s->data.string.length);
				TRY(readval(buffer, len, &ptr, s->data.string.length, s->data.string.data));
				break;
			case binary:
				//read length
				TRY(readval(buffer, len, &ptr, sizeof(tmpuint32), &tmpuint32));
				s->data.binary.length = ntohl(tmpuint32);

				//read data
				s->data.binary.data = malloc(s->data.binary.length);
				TRY(readval(buffer, len, &ptr, s->data.binary.length, s->data.binary.data));
				break;
			case fileinfo:
				fi = &s->data.fileinfo;
				//file type byte
				TRY(readval(buffer, len, &ptr, sizeof(tmpuint8), &tmpuint8));
				fi->type = tmpuint8;

				//path len
				TRY(readval(buffer, len, &ptr, sizeof(tmpuint32), &tmpuint32));
				fi->pathlen = ntohl(tmpuint32);
				///path
				fi->path = malloc(fi->pathlen);
				TRY(readval(buffer, len, &ptr, fi->pathlen, fi->path));

				//usernam len
				TRY(readval(buffer, len, &ptr, sizeof(tmpuint32), &tmpuint32));
				fi->usernamelen = ntohl(tmpuint32);
				///username
				fi->username = malloc(fi->usernamelen);
				TRY(readval(buffer, len, &ptr, fi->usernamelen, fi->username));

				//modified
				TRY(readval(buffer, len, &ptr, sizeof(tmpuint32), &tmpuint32));
				fi->modified = ntohl(tmpuint32);
				//size
				TRY(readval(buffer, len, &ptr, sizeof(tmpuint32), &tmpuint32));
				fi->size = ntohl(tmpuint32);
				//replica count
				TRY(readval(buffer, len, &ptr, sizeof(tmpuint32), &tmpuint32));
				fi->replicas = ntohl(tmpuint32);

				break;
			default:
				goto fail;
		}
	}

	return msg;
fail:
	fsmsg_free(msg);
	return NULL;
}

#undef TRY

void fsmsg_create(struct fsmsg **msg, enum fsmsg_protocol protocol) {
	*msg = malloc(sizeof(struct fsmsg));
	
	//number of ids depends on protocol
	int idcount;
	switch (protocol) {
		case FAP:
		case FCTP:
			idcount = 3;
			break;
		case DCP:
			idcount = 2;
			break;
	}

	(*msg)->ids = calloc(idcount+1, sizeof(uint64_t));
	(*msg)->msg_type = 0;
	//list of one pointer (NULL)
	(*msg)->sections = malloc(sizeof(struct msg_section *));
	(*msg)->sections[0] = NULL;
}


int fsmsg_add_section(struct fsmsg *msg, uint16_t type, union section_data data);
void fsmsg_free(struct fsmsg *msg) {
	if (!msg) return;
	FREEIF(msg->ids);

	struct msg_section *s;
	if (msg->sections) {
		for (int i=0;(s = msg->sections[i]);i++) {
			switch (s->type) {
				case binary:
					FREEIF(s->data.binary.data);
					break;
				case string:
					FREEIF(s->data.string.data);
					break;
				case fileinfo:
					FREEIF(s->data.fileinfo.path);
					FREEIF(s->data.fileinfo.username);
			}
			
			free(s);
		}
	}

	FREEIF(msg->sections);
	free(msg);
}

uint64_t swap64(uint64_t val64) {
	uint32_t low = (uint32_t) (val64&0xffffffffLL);
	uint32_t high = (uint32_t) (val64>>32);
	uint64_t new64 = ((uint64_t)htonl(low))<<32 | (uint64_t)htonl(high);
	return new64;
}

uint64_t ntohll(uint64_t net64) {
	return swap64(net64);
}
uint64_t htonll(uint64_t host64) {
	return swap64(host64);
}
