#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "util.h"

int syscallerr(int ret, char *fmtstr, ...) {
	va_list ap;
	va_start(ap, fmtstr);

	if (ret < 0) {
		fprintf(stderr, "syscall error!\n");
		vfprintf(stderr, fmtstr, ap);
		fprintf(stderr, "\n%s\n", strerror(errno));

		exit(1);
	}
	return ret;
}

FILE* logopen(const char *logpath) {
	FILE *f;
	f = fopen(logpath, "a");
	if (f==NULL) {
		syscallerr(-1 , "error opening log file %s", logpath);
	}
	return f;
}

void logwrite(FILE *logfile, char *fmtstr, ...) {
	int ret;
	va_list ap;
	va_start(ap, fmtstr);

	char logtime[128];
	time_t t = time(NULL);
	ret = strftime(logtime, 128, "%F %T %z", localtime(&t));
	if (ret<0) fprintf(stderr, "logging error!\n");
	
	ret = fputs(logtime, logfile);
	if (ret <= 0) fprintf(stderr, "logging error!\n");

	ret = vfprintf(logfile, fmtstr, ap);
	if (ret<0) fprintf(stderr, "logging error!\n");
}

int readline(char *buffer, int fp, int maxline) {
  int i=0;
  int tmp;
  while (i<maxline) {
    tmp = read(fp, &buffer[i++], 1);
    if (tmp<0) {
      syscallerr(tmp, "readline: read returned %d", tmp);
      exit(1);
    } else if (tmp==0) {
      if (i>1) {
        buffer[i-1]='\n';
        return i;
      } else {
        return 0;
      }
    } else if (buffer[i-1]=='\n') {
      return i;
    }
  }

  return 0;
}

static struct confparam *last_param, *first_param;
void add_config_param(const char *name, char **val) {
	struct confparam *new = malloc(sizeof(struct confparam));
	new->name = name;
	new->value = val;
	new->next = NULL;
	if (last_param)
		last_param->next = new;
	else
		first_param = new;
	last_param = new;
}

int read_config(const char *filename) {
	char linebuffer[MAX_LINE];
	int i;
	int fp = open(filename, O_RDONLY);
	syscallerr(fp, "open config file %s", filename);

	int len;
	while ((len = readline(linebuffer, fp, MAX_LINE))) {
		if (linebuffer[0] == '#') continue;

		char *t = strtok(linebuffer, "\t ");
		if (t == NULL) continue;

		struct confparam *cur = first_param;
		while (cur) {
			if (!strcmp(cur->name, t)) {
				i++;
				t = strtok(NULL, "\n");
				*cur->value = malloc(strlen(t));
				strcpy(*cur->value, t);
				break;
			}
			cur = cur->next;
		}
	}

	close(fp);
	return i;
}
