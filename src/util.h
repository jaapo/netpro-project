#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <signal.h>

#define NOT_IMPLEMENTED(a,b) fprintf(stderr, "not implemented!\n")

#define CLRRED "\033[31m"
#define CLREND "\033[0m"

#ifdef NDEBUG
#define DEBUGPRINT(a...)
#else
#define DEBUGPRINT(format, ...) fprintf(stderr, CLRRED "DEBUG (%s:%d, %s): \n" format "\n" CLREND, __FILE__, __LINE__, __func__, __VA_ARGS__)
#endif

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

struct addrinfo *get_server_address(char *hostname, char *servname, FILE *logfile);

//UTIL_H
#endif
