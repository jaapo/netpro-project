#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

int dcp_open(struct addrinfo *ai, uint64_t *server_id);

