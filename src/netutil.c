#include "netutil.h"
#include "util.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>

int start_listen(int port) {
	int sd, ret, one = 1;
	struct sockaddr_in bindaddr;
	socklen_t bindaddrlen = sizeof(bindaddr);

	sd = socket(AF_INET, SOCK_STREAM, 0);
	syscallerr(sd, "%s: socket() failed", __func__);

	ret = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	syscallerr(ret, "%s: setsockopt() failed", __func__);


	//TODO: use getaddrinfo for ipv6 support
	memset(&bindaddr, '\0', sizeof(bindaddr));
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindaddr.sin_port = htons(port);

	ret = bind(sd, (struct sockaddr *) &bindaddr, bindaddrlen);
	syscallerr(ret, "%s: socket() failed", __func__);

	ret = listen(sd, 5);
	syscallerr(ret, "%s: listen() failed", __func__);

	return sd;
}

