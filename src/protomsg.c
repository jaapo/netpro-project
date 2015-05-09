#include <unistd.h>
#include "protomsg.h"

#define FREEIF(x) do{if(x)free(x);}while(0)

int bufferadd(char **buffer, int *buflen, char **ptr, const void *data, int datalen) {
	int curlen = *ptr-*buffer;

	if (curlen+datalen > *buflen) {
		*buflen += datalen+64;
		*buffer = realloc(*buffer, *buflen);
		*ptr = *buffer+curlen;
	}

	memcpy(*ptr, data, datalen);
	*ptr += datalen;

	return datalen;
}

int fsmsg_to_buffer(struct fsmsg *msg, char **buffer, enum fsmsg_protocol protocol) {
	char *buf, *data;
	int bufsize = 1024;
	int len = 0;

	buf = malloc(bufsize);
	data = buf;

	uint64_t tmpuint64;
	uint32_t tmpuint32;
	uint16_t tmpuint16;
	uint8_t tmpuint8;
	int32_t tmpint32;

	//transaction id
	tmpuint64 = htonll(msg->tid);
	len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint64, sizeof(tmpuint64));
	
	//other ids
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
	for (int i=0;i<idcount;i++) {
		tmpuint64 = msg->ids[i];
		len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint64, sizeof(tmpuint64));
	}

	//message type
	tmpuint16 = htons(msg->msg_type);
	len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint16, sizeof(tmpuint16));
	
	//sections
	struct msg_section *section;
	for (int i=0;(section = msg->sections[i]);i++) {
		tmpuint16 = htons(section->type);
		len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint16, sizeof(tmpuint16));

		switch ((enum section_type) section->type) {
			case integer:
				tmpint32 = htonl(section->data.integer);
				len += bufferadd(&buf, &bufsize, &data, (void *)&tmpint32, sizeof(tmpint32));
				break;
			case string:
				tmpuint32 = htonl(section->data.string.length);
				len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				len += bufferadd(&buf, &bufsize, &data, (void *)section->data.string.data, section->data.string.length);
				break;
			case binary:
				tmpuint32 = htonl(section->data.binary.length);
				len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				len += bufferadd(&buf, &bufsize, &data, (void *)section->data.binary.data, section->data.binary.length);
				break;
			case fileinfo:
				tmpuint8 = section->data.fileinfo.type;
				len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint8, sizeof(tmpuint8));
				
				tmpuint32 = htonl(section->data.fileinfo.pathlen);
				len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				len += bufferadd(&buf, &bufsize, &data, (void *)section->data.fileinfo.path, section->data.fileinfo.pathlen);

				tmpuint32 = htonl(section->data.fileinfo.usernamelen);
				len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				len += bufferadd(&buf, &bufsize, &data, (void *)section->data.fileinfo.username, section->data.fileinfo.usernamelen);
				
				tmpuint32 = htonl(section->data.fileinfo.modified);
				len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				tmpuint32 = htonl(section->data.fileinfo.size);
				len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				tmpuint32 = htonl(section->data.fileinfo.replicas);
				len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint32, sizeof(tmpuint32));
				break;
			case nonext:
				tmpuint16 = 0;
				len += bufferadd(&buf, &bufsize, &data, (void *)&tmpuint16, sizeof(tmpuint16));
				break;
			default:
				break;
		}
	}

	*buffer = buf;
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
	ptr = buffer;

	struct fsmsg *msg;
	msg = fsmsg_create(protocol);

	//read ids, transaction id is first
	TRY(readval(buffer, len, &ptr, sizeof(msg->tid), &msg->tid));

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
		TRY(readval(buffer, len, &ptr, sizeof(msg->tid), &msg->ids[i]));
	}
	
	//message type
	TRY(readval(buffer, len, &ptr, sizeof(msg->msg_type), &msg->msg_type));
	
	//sections
	uint16_t sectype;
	int sectioncount;
	struct msg_section *s;
	while(1) {
		TRY(readval(buffer, len, &ptr, sizeof(sectype), &sectype));
		if (ntohs(sectype) == nonext) break;

		sectioncount++;
		msg->sections = realloc(msg->sections, (sectioncount+1)*sizeof(struct msg_section*));
		msg->sections[sectioncount] = NULL;
		s = malloc(sizeof(struct msg_section));
		msg->sections[sectioncount-1] = s;

		s->type = sectype;

		switch ((enum section_type) ntohs(sectype)) {
			struct fileinfo_sect *fi;
			case integer:
				TRY(readval(buffer, len, &ptr, sizeof(s->data.integer), &s->data.integer));
				break;
			case string:
				//read length
				TRY(readval(buffer, len, &ptr, sizeof(s->data.string.length), &s->data.string.length));

				//read data
				s->data.string.data = malloc(s->data.string.length);
				TRY(readval(buffer, len, &ptr, s->data.string.length, s->data.string.data));
				break;
			case binary:
				//read length
				TRY(readval(buffer, len, &ptr, sizeof(s->data.binary.length), &s->data.binary.length));

				//read data
				s->data.binary.data = malloc(s->data.binary.length);
				TRY(readval(buffer, len, &ptr, s->data.binary.length, s->data.binary.data));
				break;
			case fileinfo:
				fi = &s->data.fileinfo;
				//file type byte
				TRY(readval(buffer, len, &ptr, sizeof(fi->type), &fi->type));

				//path
				TRY(readval(buffer, len, &ptr, sizeof(fi->pathlen), &fi->pathlen));
				fi->path = malloc(fi->pathlen);
				TRY(readval(buffer, len, &ptr, fi->pathlen, fi->path));

				//username
				TRY(readval(buffer, len, &ptr, sizeof(fi->usernamelen), &fi->usernamelen));
				fi->username = malloc(fi->usernamelen);
				TRY(readval(buffer, len, &ptr, fi->usernamelen, fi->username));

				TRY(readval(buffer, len, &ptr, sizeof(fi->modified), &fi->modified));
				TRY(readval(buffer, len, &ptr, sizeof(fi->size), &fi->size));
				TRY(readval(buffer, len, &ptr, sizeof(fi->replicas), &fi->replicas));

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

struct fsmsg* fsmsg_from_socket(int sd, enum fsmsg_protocol protocol) {
	struct fsmsg *msg;
	msg = fsmsg_create(protocol);

	//read ids, transaction id is first
	TRY(read(sd, &msg->tid, sizeof(msg->tid)));

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
		TRY(read(sd, &msg->ids[i], sizeof(msg->tid)));
	}
	
	//message type
	TRY(read(sd, &msg->msg_type, sizeof(msg->msg_type)));
	
	//sections
	uint16_t sectype;
	int sectioncount;
	struct msg_section *s;
	while(1) {
		TRY(read(sd, &sectype, sizeof(sectype)));
		if (sectype == nonext) break;

		sectioncount++;
		msg->sections = realloc(msg->sections, (sectioncount+1)*sizeof(struct msg_section*));
		msg->sections[sectioncount] = NULL;
		s = malloc(sizeof(struct msg_section));
		msg->sections[sectioncount-1] = s;

		s->type = (enum section_type) ntohs(sectype);

		switch (s->type) {
			struct fileinfo_sect *fi;
			case integer:
				TRY(read(sd, &s->data.integer, sizeof(s->data.integer)));
				break;
			case string:
				//read length
				TRY(read(sd, &s->data.string.length, sizeof(s->data.string.length)));

				//read data
				s->data.string.data = malloc(s->data.string.length);
				TRY(read(sd, s->data.string.data, s->data.string.length));
				break;
			case binary:
				//read length
				TRY(read(sd, &s->data.binary.length, sizeof(s->data.binary.length)));

				//read data
				s->data.binary.data = malloc(s->data.binary.length);
				TRY(read(sd, s->data.binary.data, s->data.binary.length));
				break;
			case fileinfo:
				fi = &s->data.fileinfo;
				//file type byte
				TRY(read(sd, &fi->type, sizeof(fi->type)));

				//path
				TRY(read(sd, &fi->pathlen, sizeof(fi->pathlen)));
				fi->path = malloc(ntohl(fi->pathlen));
				TRY(read(sd, fi->path, fi->pathlen));

				//usernam
				TRY(read(sd, &fi->usernamelen, sizeof(fi->usernamelen)));
				fi->username = malloc(ntohl(fi->usernamelen));
				TRY(read(sd, fi->username, fi->usernamelen));

				TRY(read(sd, &fi->modified, sizeof(fi->modified)));
				TRY(read(sd, &fi->size, sizeof(fi->size)));
				TRY(read(sd, &fi->replicas, sizeof(fi->replicas)));

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

struct fsmsg* fsmsg_create(enum fsmsg_protocol protocol) {
	struct fsmsg *msg = malloc(sizeof(struct fsmsg));
	
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

	msg->ids = calloc(idcount+1, sizeof(uint64_t));
	msg->msg_type = 0;
	//list of one pointer (NULL)
	msg->sections = malloc(sizeof(struct msg_section *));
	msg->sections[0] = NULL;

	return msg;
}


void fsmsg_add_section(struct fsmsg *msg, uint16_t type, union section_data *data) {
	int sectioncount;

	while (msg->sections[sectioncount++]);

	msg->sections = realloc(msg->sections, sizeof(struct msg_section*)*sectioncount+1);
	msg->sections[sectioncount] = NULL;
	msg->sections[sectioncount-1] = malloc(sizeof(struct msg_section));

	struct msg_section *sec = msg->sections[sectioncount-1];
	sec->type = type;

	switch ((enum section_type) type) {
		struct fileinfo_sect *fi;
		case integer:
			sec->data.integer = data->integer;
			break;
		case string:
			sec->data.string.length = data->string.length;
			sec->data.string.data = malloc(data->string.length);
			memcpy(sec->data.string.data, data->string.data, data->string.length);
			break;
		case binary:
			sec->data.binary.length = data->binary.length;
			sec->data.binary.data = malloc(data->binary.length);
			memcpy(sec->data.binary.data, data->binary.data, data->binary.length);
			break;
		case fileinfo:
			fi = &sec->data.fileinfo;
			//file type byte
			fi->type = data->fileinfo.type;

			//path len
			fi->pathlen = data->fileinfo.pathlen;
			///path
			fi->path = malloc(fi->pathlen);
			memcpy(fi->path, data->fileinfo.path, fi->pathlen);

			//usernam len
			fi->usernamelen = data->fileinfo.usernamelen;
			///username
			fi->username = malloc(fi->usernamelen);
			memcpy(fi->username, data->fileinfo.username, fi->usernamelen);

			//modified
			fi->modified = data->fileinfo.modified;
			//size
			fi->size = data->fileinfo.size;
			//replica count
			fi->replicas = data->fileinfo.replicas;
			break;
		case nonext:
			//no need to copy data
			break;
	}
}
void fsmsg_free_section(struct msg_section *s) {
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

void fsmsg_free(struct fsmsg *msg) {
	if (!msg) return;
	FREEIF(msg->ids);

	struct msg_section *s;
	if (msg->sections) {
		for (int i=0;(s = msg->sections[i]);i++) {
			fsmsg_free_section(s);
		}
	}

	FREEIF(msg->sections);
	free(msg);
}

void fsmsg_fileinfo_copy(struct fileinfo_sect* dst, struct fileinfo_sect* src) {
	memcpy(dst, src, sizeof(struct fileinfo_sect));
	dst->path = malloc(dst->pathlen);
	memcpy(dst->path, src->path, dst->pathlen);

	dst->username = malloc(dst->usernamelen);
	memcpy(dst->username, src->username, dst->usernamelen);
}

struct msg_section *fsmsg_section_copy(struct msg_section *s) {
	struct msg_section *new;
	new = malloc(sizeof(struct msg_section));
	new->type = s->type;
	memcpy(new, s, sizeof(struct msg_section));
	
	switch (new->type) {
		case string:
			new->data.string.data = malloc(new->data.string.length);
			memcpy(new->data.string.data, s->data.string.data, new->data.string.length);
			break;
		case binary:
			new->data.binary.data = malloc(new->data.binary.length);
			memcpy(new->data.binary.data, s->data.binary.data, new->data.binary.length);
			break;
		case fileinfo:
			fsmsg_fileinfo_copy(&new->data.fileinfo, &s->data.fileinfo);
			break;
	}

	return new;
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
