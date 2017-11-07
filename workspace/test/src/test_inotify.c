/*
 * test_inotify.c
 *
 *  Created on: 2017. 11. 7.
 *      Author: yang
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

void t_inotify()
{
	int	fd, wd;
	//struct inotify_event *evt;

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
