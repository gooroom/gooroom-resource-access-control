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

#include "grac_config.h"
#include "grac_rule.h"
#include "grac_resource.h"
#include "cutility.h"
#include "sys_user.h"
#include "sys_etc.h"


static void test()
{
	const char	*rule_path;

	rule_path = grac_config_path_default_grac_rules();

	GracRule *grac_rule = grac_rule_alloc();
	if (rule_path != NULL) {
		gboolean res;
		res = grac_rule_load(grac_rule, (gchar*)rule_path);
		if (res == FALSE) {
			printf("%s() : load rule error : %s", __FUNCTION__, rule_path);
			return;
		}
		else {
			grac_rule_apply(grac_rule);
		}
	}

}

int main()
{
/*
	char name[1024];
	gboolean done;

	done = sys_user_get_login_name_by_api(name, sizeof(name));

	printf("%d: %s\n", (int)done, name);

*/

	test();

	return EXIT_SUCCESS;
}


