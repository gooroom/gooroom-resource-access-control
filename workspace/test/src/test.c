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

#include <cups/cups.h>

#include "grm_log.h"
#include "grac_config.h"
#include "grac_rule.h"
#include "grac_resource.h"
//#include "grac_block_device.h"
#include "cutility.h"
#include "sys_user.h"
#include "sys_file.h"
#include "sys_etc.h"

#include <sys/inotify.h>

void t_inotify()
{
	int	fd, wd;
	struct inotify_event *evt;

	fd = inotify_init();

	if (fd > 0) {
		wd = inotify_add_watch(fd, "/etc/gooroom/grac.d/kkk", IN_CLOSE_WRITE | IN_CLOSE_NOWRITE);
		if (wd > 0) {
			while (1) {
				int n;
				char	buf[1024];
				n = read(fd, buf, sizeof(buf));
				if (n <= 0)
					break;
				if (n > 0) {
					buf[n] = 0;
					printf("yes : %s\n", buf);
				}
				fgets(buf, sizeof(buf), stdin);
				printf("----%s", buf);
				if (buf[0] == 'a') {
					inotify_rm_watch(fd, wd);
					close(fd);
					printf("mmmm : %s\n", strerror(errno));
				}
			}
			printf("stop\n");
		}
		else {
			printf("error : wd\n");
			close(fd);
		}
	}
	else {
		printf("error : fd\n");
	}
}

int main(int argc, char *argv[])
{

	t_inotify();

//printf("check - %d\n", (int) grac_udev_check_existing("sda"));

/*
	if (argc > 1) {
		grac_eject(argv[1]);
	}
*/
//	grac_eject("vda");
//	printf("\n");
//	grac_eject("vda1");

// 	t_block();

//	test_udev();

	return EXIT_SUCCESS;
}



