/*
 * test_socket.c
 *
 *  Created on: 2017. 7. 14.
 *      Author: yang
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>


void test_addr()
{
	int		sd;
	int		port = 80;
	struct sockaddr_in  server_addr;
	char	*addr = "www.hancom.com";

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		printf("Error: can't create socket");
	}
	else {
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		server_addr.sin_addr.s_addr = inet_addr(addr);
		if (server_addr.sin_addr.s_addr == -1) {
			struct hostent *host = gethostbyname(addr);
			if (host == NULL) {
				printf("Error: can't get hostname");
				return;
			}
			bcopy (host->h_addr_list[0], (char*)&server_addr.sin_addr, host->h_length);
		}

		int i;
		char *ptr = (char*)&server_addr.sin_addr;
		for (i=0; i<4; i++)
			printf("%d-", ptr[i] & 0x0ff);
		printf("\n");

		int n = connect(sd, (struct sockaddr*)&server_addr, sizeof(server_addr));
		if (n == -1) {
			printf("Error: can't connect : %s", strerror(errno));
		}
	}
	printf("\n");
}

void test1()
{
	struct protoent *ent;

	ent = getprotoent();
	while (ent) {
		printf("%d %s\n", ent->p_proto, ent->p_name);
		ent = getprotoent();
	}
	printf("\n");

	ent = getprotobyname("ip");
	if (ent)
		printf("%d %s\n", ent->p_proto, ent->p_name);
	ent = getprotobyname("xxx");
	if (ent)
		printf("%d %s\n", ent->p_proto, ent->p_name);

	ent = getprotobyname("tcp");	// match by name
	if (ent)
		printf("%d %s\n", ent->p_proto, ent->p_name);
	ent = getprotobyname("TCP");   // match by alias
	if (ent)
		printf("%d %s\n", ent->p_proto, ent->p_name);
	ent = getprotobyname("TcP");   // error
	if (ent)
		printf("%d %s\n", ent->p_proto, ent->p_name);

	char	tmp[128];
	char	addr4[4] = { 127, 0, 1, 2 };
	struct in_addr addr;
	memcpy(&addr, addr4, sizeof(addr));
	printf("%s\n", inet_ntoa(addr) );
	printf("%s\n", inet_ntop(AF_INET, &addr, tmp, sizeof(tmp)) );


	struct in6_addr addr6;
	printf("%s\n", inet_ntop(AF_INET6, &addr6, tmp, sizeof(tmp)) );
	printf("%s\n", tmp);

}
