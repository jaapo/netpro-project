#include <stdio.h>
#include <signal.h>

#define MAX_LINE 1024

struct confparam {
	const char *name;
	char **value;
	struct confparam *next;
};

int syscallerr(int ret, char *fmtstr, ...);
int readline(char *buffer, int fp, int maxline);
void add_config_param(const char *name, char **val);
int read_config(const char *filename);
FILE* logopen(const char *logpath);
void logwrite(FILE *logfile, char *fmtstr, ...);
