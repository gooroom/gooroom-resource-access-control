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
