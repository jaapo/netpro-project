#ifndef NETUTIL_H
#define NETUTIL_H
#include <stdio.h>
#include <sys/types.h>

int start_listen();
int recvfile(int sd, int fd, int len);
struct addrinfo *get_server_address(char *hostname, char *servname, FILE *logfile);

//NETUTIL_H
#endif
