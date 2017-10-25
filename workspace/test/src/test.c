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

static void _recover_configurationValue(char *buf, int buf_size)
{
	char	*ptr;
	int		i, n, ch, cnt;
	char	*attr = "bConfigurationValue";

	ptr = c_stristr(buf, attr, buf_size);
	if (ptr) {
		ptr += c_strlen(attr, 256);
		ptr++;
		if (*ptr >= '0' && *ptr <= '9') {
			cnt = *ptr - '0';
			ptr++;
		}
		else {
			cnt = 0;
		}

		ptr = c_stristr(buf, "P=", buf_size);
		if (ptr) {
			ptr += 2;
			char	dev_path[512];
			for (i=0; i<sizeof(dev_path)-1; i++) {
				ch = ptr[i] & 0x0ff;
				if (ch <= 0x02)
					break;
				dev_path[i] = ch;
			}
			if (i>0 && dev_path[i-1] == '/')
				i--;
			dev_path[i] = 0;

			if (cnt > 0) {
				n = i;
				for (i=n-1; i>=0; i--) {
					if (dev_path[i] == '/') {
						dev_path[i] = 0;
						cnt--;
						if (cnt == 0)
							break;
					}
				}
			}

			char	cmd[1024];
			snprintf(cmd, sizeof(cmd), "echo 1 > %s/%s", dev_path, attr);

			printf("%s --> %s\n", buf, cmd);
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

	/*
	_recover_configurationValue("bConfigurationValue.3 P=/sys/a/b/c/d/e/f", 256);
	_recover_configurationValue("bConfigurationValue.4 P=/sys/a/b/c/d/e/f", 256);
	_recover_configurationValue("bConfigurationValue  P=/sys/a/b/c/d/e/f", 256);

	_recover_configurationValue("R=cdrom M=bConfigurationValue.3 P=/sys/a/b/c/d/e/f", 256);
	_recover_configurationValue("R=cdrom M=bConfigurationValue.4 P=/sys/a/b/c/d/e/f", 256);
	_recover_configurationValue("R=cdrom M=bConfigurationValue  P=/sys/a/b/c/d/e/f", 256);
*/

	return EXIT_SUCCESS;
}


