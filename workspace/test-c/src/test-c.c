/*
 ============================================================================
 Name        : test-c.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>

int main(int argc, char *argv[])
{
	printf("%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);

	int	val[7], n, i;
	char *data;

	data = "12:34";
	n = sscanf(data,  "%x:%x", &val[0], &val[1], &val[2]);
	printf("%s -> %d -> ", data, n);
	for (i=0; i<n; i++) printf("%x ", val[i]);
	printf("\n");

	data = "12:34";
	n = sscanf(data,  "%x:%x:%x", &val[0], &val[1], &val[2]);
	printf("%s -> %d -> ", data, n);
	for (i=0; i<n; i++) printf("%x ", val[i]);
	printf("\n");

	data = "1256:34:56";
	n = sscanf(data,  "%x:%x", &val[0], &val[1], &val[2]);
	printf("%s -> %d -> ", data, n);
	for (i=0; i<n; i++) printf("%x ", val[i]);
	printf("\n");

	data = "1256:34:56";
		n = sscanf(data,  "%x:%x:%x", &val[0], &val[1], &val[2]);
		printf("%s -> %d -> ", data, n);
		for (i=0; i<n; i++) printf("%x ", val[i]);
		printf("\n");

	data = "xyz:34:56";
	n = sscanf(data,  "%x:%x:%x", &val[0], &val[1], &val[2]);
	printf("%s -> %d -> ", data, n);
	for (i=0; i<n; i++) printf("%x ", val[i]);
	printf("\n");


	if (argc < 2)
		return 1;



	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_in* sin;
	struct sockaddr_in6* sin6;
	char	buf[256];

	memset(&hints, 0, sizeof(hints));

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int err = getaddrinfo(argv[1], NULL, NULL, &res);
	printf("result : %d\n", err);

	if (err == 0) {
		struct addrinfo *iter;
		for (iter=res; iter!=NULL; iter=iter->ai_next) {
			if (iter->ai_family == AF_INET) {
				sin = (void*)iter->ai_addr;
				inet_ntop(iter->ai_family, &sin->sin_addr, buf, sizeof(buf));
				printf("IPv4 --- %s", buf);
			}
			else if (iter->ai_family == AF_INET6) {
				sin6 = (void*)iter->ai_addr;
				inet_ntop(iter->ai_family, &sin6->sin6_addr, buf, sizeof(buf));
				printf("IPv6 ---");
			}
			else {
				printf("Not supported\n");
			}
			printf("\n");
		}
	}

	return EXIT_SUCCESS;
}
