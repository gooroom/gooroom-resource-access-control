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

#include "grac_rule.h"
#include "grac_resource.h"
#include "cutility.h"
#include "sys_user.h"

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
	GracRule	*grac_rule = grac_rule_alloc();

	grac_rule_load(grac_rule, NULL);

//	grac_rule_apply(grac_access);

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
	grac_rule_free(&grac_rule);
}

//#include <libudev.h>

void test_resource()
{
	char	*res_name;
	int		res_id;
	char *perm_name;

	res_name = grac_resource_find_first_resource();
	while (res_name) {
		res_id = grac_resource_get_resource_id(res_name);
		printf("%-10s\t--> ", res_name);
		perm_name = grac_resource_find_first_permission(res_id);
		while (perm_name) {
			printf("%s\t", perm_name);
			perm_name = grac_resource_find_next_permission();
		}
		printf("\n");
		res_name = grac_resource_find_next_resource();
	}
}

#include <libnotify/notify.h>

void notify(char *msg)
{
	notify_init("GRAC warining");
	NotifyNotification *noti = notify_notification_new("Hello", msg, "this is a test");
	notify_notification_show(noti, NULL);
	g_object_unref(G_OBJECT(noti));
	notify_uninit();
}

void t_notify()
{
	char buf[1024];

	while (1) {
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		if (strlen(buf) <= 1)
			break;
		notify(buf);
	}
}

int main()
{
	char name[1024];
	gboolean done;

	done = sys_user_get_login_name_by_api(name, sizeof(name));

	printf("%d: %s\n", (int)done, name);

	char tmp_file[1024];
	c_strcpy(tmp_file, "/tmp/grac_tmp_XXXXXX", sizeof(tmp_file));

	int res;
	res = mkstemp(tmp_file);
	printf("%s : %d\n", tmp_file, res);

	c_strcpy(tmp_file, "/123/456/xyz/grac_tmp_XXXXXX", sizeof(tmp_file));
	res = mkstemp(tmp_file);
	printf("%s : %d\n", tmp_file, res);


	return EXIT_SUCCESS;
}


