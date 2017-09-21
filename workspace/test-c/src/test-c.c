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


void tmp_file()
{
	char tmp_file[1024];
	strncpy(tmp_file, "/tmp/grac_tmp_XXXXXX", sizeof(tmp_file));

	int res;
	res = mkstemp(tmp_file);
	printf("%s : %d\n", tmp_file, res);

	strncpy(tmp_file, "/123/456/xyz/grac_tmp_XXXXXX", sizeof(tmp_file));
	res = mkstemp(tmp_file);
	printf("%s : %d\n", tmp_file, res);

}

void t_scanf()
{
	int	val[7], n, i;
	char *data;
	char *form;

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

	data = "NAME=\"sr0\" TYPE=\"rom\" RM=\"1\" RO=\"0\" MOUNTPOINT=\"\" HOTPLUG=\"1\" SUBSYSTEMS=\"block:scsi:pci\"";
	form = "NAME=\"%s\" TYPE=%s RM=%d RO=%d MOUNTPOINT=%s HOTPLUG=%d SUBSYSTEMS=%s";

	char	name[256];
	char	type[256];
	char	mount[256];
	char	subsys[256];
	int	  ro, rm, hot;

	n = sscanf(data, form, name, type, &rm, &ro, mount, &hot, subsys);
	printf("%d\n", n);
	printf("%s\n", name);

}

void t_addrinfo(char *addr)
{
	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_in* sin;
	struct sockaddr_in6* sin6;
	char	buf[256];

	memset(&hints, 0, sizeof(hints));

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int err = getaddrinfo(addr, NULL, NULL, &res);
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
}


struct _main {
	int	m1;
	struct _sub1 {
		int	s1;
	} sub;
	struct _sub2 {
		int	s2;
	} ;
};

int main(int argc, char *argv[])
{
//	printf("%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);


	printf("%d\n", sizeof(struct _main));
	printf("%d\n", sizeof(struct .sub1));

	return EXIT_SUCCESS;
}
