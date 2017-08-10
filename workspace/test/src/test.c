/*
 ============================================================================
 Name        : test.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "grac_access.h"


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

void test2()
{
	GracAccess	*grac_access = grac_access_alloc();

	grac_access_load(grac_access, NULL);

	grac_access_apply(grac_access);

//	GracAccess *access = grac_access_get_access(grac_access);
/*
	gchar *attr, *val;

	attr = grac_access_find_first_attr(grac_access);
	while (attr) {
		val = grac_access_get_attr(grac_access, attr);
		printf("%s - %s \n", attr, val);
		attr = grac_access_find_next_attr(grac_access);
	}

*/
	grac_access_free(&grac_access);
}

#include <libudev.h>

void test()
{

}

int main()
{
	test();

	return EXIT_SUCCESS;
}

