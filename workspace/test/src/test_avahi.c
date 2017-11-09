/*
 * test_avahi.c
 *
 *  Created on: 2017. 11. 1.
 *      Author: yang
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
#include <sys/syscall.h>

#include <cups/cups.h>

#include "grac_log.h"
#include "grac_config.h"
#include "grac_rule.h"
#include "grac_resource.h"
#include "grac_printer.h"
#include "cutility.h"
#include "sys_user.h"
#include "sys_etc.h"


void t_avahi()
{
	if (grac_printer_init() == FALSE) {
		printf("Init error\n");
		return;
	}


	char	buf[256];
	while (1) {
		if (fgets(buf, 256, stdin) == NULL)
			break;
		c_strtrim(buf, 256);
		if (c_strmatch(buf, "allow")) {
			printf("allow\n");
			grac_printer_apply(TRUE);
		}
		else if (c_strmatch(buf, "disallow")) {
			printf("disallow\n");
			grac_printer_apply(FALSE);
		}
		else if (c_strmatch(buf, "exit")) {
			printf("exit\n");
			break;
		}
		else {
			printf("loop\n");
		}
	}

	if (grac_printer_end() == FALSE) {
		printf("Ending error\n");
		return;
	}
}


void test_list_by_cups()
{
	cups_dest_t *printer_dests = NULL;
	int printer_count = 0;

	// printer_count = cupsGetDests2(CUPS_HTTP_DEFAULT, &printer_dests); same results
	printer_count = cupsGetDests(&printer_dests);
	if (printer_count == 0) {
		printf("Not found printer\n");
		return;
	}

	int idx;
	for (idx=0; idx < printer_count; idx++)
		printf("%d : %s\n", idx, printer_dests[idx].name);

	if (printer_dests != NULL) {
			cupsFreeDests(printer_count, printer_dests);
			printer_dests = NULL;
			printer_count = 0;
	}

	printf("\n");
	test_list_by_cups();

}

struct _AvahiCtrl {
	gboolean	request_stop;
	gboolean	normal_stop;
	gboolean	on_avahi_thread;
	FILE	*p_fp;
	pid_t  p_pid;
};

static struct _AvahiCtrl  AvahiCtrl = { 0 };

static gboolean get_key_data(char *str, char *key, int max, char* data, int data_size)
{
	gboolean done = FALSE;
	char 	*ptr;
	int		key_len, idx;

	key_len = c_strlen(key, max);
	if (key_len > 0) {
		ptr = c_strstr(str, key, max);
		if (ptr) {
			ptr += key_len;
			idx = c_strchr_idx(ptr, '\"', max);
			if (idx > 0 && idx < data_size-1) {
				c_memset(data, 0, data_size);
				c_memcpy(data, ptr, idx);
				done = TRUE;
			}
		}
	}

	return done;
}

static void* avahi_thread(void *data)
{
	struct _AvahiCtrl *ctrl = (struct _AvahiCtrl*)data;
	char	buf[1024];
	char	*key;
	char	rp[256], type[256], url[1024];
	char	prt_name[256];
	FILE	*fp;
	gboolean res;
	char	*cmd = "/usr/bin/avahi-browse -arp";
	pid_t	pid;

	grac_log_debug("start avahi_thread : pid = %d", getpid());

	fp = sys_popen(cmd, "r", &pid);
	if (fp == NULL) {
		grac_log_debug("%s() : can't run - %s", __FUNCTION__, cmd);
		return NULL;
	}
	grac_log_debug("avahi_thread : create child process = %d", pid);

	/*
	int fd = fileno(fp);
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		grac_log_debug("%s() : fcntl : not set NONBLOCK: %s", __FUNCTION__, strerror(errno));
		pclose(fp);
		return NULL;
	}
*/
	ctrl->p_fp = fp;
	ctrl->p_pid = pid;
	ctrl->on_avahi_thread = TRUE;
	ctrl->request_stop = FALSE;

	while (ctrl->request_stop == FALSE) {

			if (fgets(buf, sizeof(buf), fp) == NULL) {
				g_usleep(100*1000);
				continue;
			}

			grac_log_debug("%s", buf);

			if (buf[0] != '+' && buf[0] != '=')
				continue;

			key = "\"rp=";
			res = get_key_data(buf, key, sizeof(buf), rp, sizeof(rp));
			if (res == FALSE)
				continue;

			key = "\"ty=";
			res = get_key_data(buf, key, sizeof(buf), type, sizeof(type));
			if (!res)
				type[0] = 0;
			key = "\"adminurl=";
			res = get_key_data(buf, key, sizeof(buf), url, sizeof(url));
			if (!res)
				url[0] = 0;

			char *ptr = c_strstr(rp, "/", sizeof(rp));
			if (ptr == NULL) {
				grac_log_error("%s() : inavlid printer information : %s", __FUNCTION__, rp);
			}
			else {
				c_strcpy(prt_name, ptr+1, sizeof(prt_name));
				grac_log_debug("%s() : catch new printer : %s", __FUNCTION__, rp);
				//_delete_printer(prt_name);
				//_delete_current_printer_list();
			}
	}
	grac_log_debug("out of avahi thread");

	if (ctrl->p_fp) {
		fp = ctrl->p_fp;
		pid = ctrl->p_pid;
		ctrl->p_fp = NULL;
		ctrl->p_pid = 0;

		sys_pclose(fp, pid);
	}

	ctrl->on_avahi_thread = FALSE;

	grac_log_debug("stop avahi thread : pid = %d tid=%d", sys_getpid(), sys_gettid());

	return NULL;
}


gboolean t_start_printer_monitor()
{
	c_memset(&AvahiCtrl, 0, sizeof(AvahiCtrl));

	int	res;
	pthread_t thread;

	res = pthread_create(&thread, NULL, avahi_thread, (void*)&AvahiCtrl);
	if (res != 0) {
		grac_log_error("can't create avahi_thread : %d", res);
		return FALSE;
	}

	return TRUE;
}

void t_stop_printer_monitor()
{
	AvahiCtrl.normal_stop = TRUE;
	AvahiCtrl.request_stop = TRUE;

	if (AvahiCtrl.p_fp) {
		FILE *fp = AvahiCtrl.p_fp;
		int	pid = AvahiCtrl.p_pid;
		AvahiCtrl.p_fp = NULL;
		AvahiCtrl.p_pid = 0;

		sys_pclose(fp, pid);
	}

	while (AvahiCtrl.on_avahi_thread) {
		g_usleep(10*1000);
	}
}

