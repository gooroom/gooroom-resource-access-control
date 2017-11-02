/*
 * t_recover.c
 *
 *  Created on: 2017. 10. 26.
 *      Author: yang
 */

#include "cutility.h"

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
				if (ch <= 0x020)
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


void t_recover()
{

	_recover_configurationValue("bConfigurationValue.3 P=/sys/a/b/c/d/e/f", 256);
	_recover_configurationValue("bConfigurationValue.4 P=/sys/a/b/c/d/e/f", 256);
	_recover_configurationValue("bConfigurationValue  P=/sys/a/b/c/d/e/f", 256);

	_recover_configurationValue("R=cdrom M=bConfigurationValue.3 P=/sys/a/b/c/d/e/f", 256);
	_recover_configurationValue("R=cdrom M=bConfigurationValue.4 P=/sys/a/b/c/d/e/f", 256);
	_recover_configurationValue("R=cdrom M=bConfigurationValue  P=/sys/a/b/c/d/e/f", 256);

}
