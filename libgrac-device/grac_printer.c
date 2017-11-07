/*
 ============================================================================
 Name        : grac_printer.c
 Author      : 
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */

/**
  @file 	 	grac_printer.c
  @brief		프린터 관리 ( cups와 avahi 이용)
  @details	\n
  				헤더파일 :  	grac_printer.h	\n
  				라이브러리:	libgrac-device.so
 */

#include "grac_printer.h"
#include "grm_log.h"
#include "cutility.h"
#include "sys_file.h"
#include "sys_etc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cups/cups.h>

#include <glib.h>
#include <glib/gstdio.h>

struct _AvahiCtrl {
	gboolean	request_stop;
	gboolean	normal_stop;
	gboolean	on_avahi_thread;
	FILE	*p_fp;
	pid_t  p_pid;

	int		i_fd;
	int		i_wd;
};

static struct _AvahiCtrl  G_AvahiCtrl = { 0 };

static gboolean _delete_current_printer_list();


#include <sys/inotify.h>

static void* _watch_ppd_thread(void *data)
{
	struct _AvahiCtrl *ctrl = (struct _AvahiCtrl*)data;
	int	fd, wd;

	fd = inotify_init();
	if (fd <= 0) {
		grm_log_error("%s() : can't init notify", __FUNCTION__);
		return NULL;
	}

	wd = inotify_add_watch(fd, "/etc/cups/ppd", IN_CLOSE_WRITE);
	if (wd <= 0) {
		grm_log_error("%s() : can't add watch", __FUNCTION__);
		close(fd);
		return NULL;
	}
	ctrl->i_fd = fd;
	ctrl->i_wd = wd;

	int n;
	char	buf[1024];

	while (1) {
		n = read(fd, buf, sizeof(buf));
		if (n <= 0)
				break;

		grm_log_debug("%s() : check and delete", __FUNCTION__);
		_delete_current_printer_list();
	}

	if (ctrl->i_fd > 0 && ctrl->i_wd > 0) {
		inotify_rm_watch(ctrl->i_fd, ctrl->i_wd);
		close(ctrl->i_fd);
	}

	grm_log_debug("%s() : end of watching ppd", __FUNCTION__);

	return NULL;
}

static gboolean _start_watch_ppd()
{
	int	res;
	pthread_t thread;

	G_AvahiCtrl.i_fd = 0;
	G_AvahiCtrl.i_wd = 0;

	res = pthread_create(&thread, NULL, _watch_ppd_thread, (void*)&G_AvahiCtrl);
	if (res != 0) {
		grm_log_error("can't create watch_ppd_thread : %d", res);
		return FALSE;
	}

	return TRUE;
}

static void _stop_watch_ppd()
{
	if (G_AvahiCtrl.i_fd) {
		int fd, wd;
		fd = G_AvahiCtrl.i_fd;
		wd = G_AvahiCtrl.i_wd;
		G_AvahiCtrl.i_fd = 0;
		G_AvahiCtrl.i_wd = 0;
		inotify_rm_watch(fd, wd);
		close(fd);
	}
}

static gboolean _existing_printer(char *prt_name, gboolean cups, gboolean ppd)
{
	gboolean found = FALSE;

	if (cups) {
		cups_dest_t *printer_dests = NULL;
		int printer_count = 0;
		int idx;
		printer_count = cupsGetDests(&printer_dests);
		for (idx=0; idx < printer_count; idx++) {
			if (c_strmatch(prt_name, printer_dests[idx].name)) {
				found = TRUE;
				break;
			}
		}

		cupsFreeDests(printer_count, printer_dests);
	}

	if (ppd) {
		if (found == FALSE) {
			char ppd_file[1024];
			g_snprintf(ppd_file, sizeof(ppd_file), "/etc/cups/ppd/%s.ppd", prt_name);
			if (sys_file_is_existing(ppd_file)) {
				grm_log_debug("%s() : no cups dest but existing ppd", __FUNCTION__, prt_name);
				found = TRUE;
			}
		}
	}

	return found;
}


static gboolean _delete_printer(char *prt_name)
{
	gboolean done = TRUE;
	char	cmd[1024];

	if (_existing_printer(prt_name, TRUE, TRUE) == FALSE) {
		grm_log_error("%s() : not existing printer", __FUNCTION__);
		return TRUE;
	}

	g_snprintf(cmd, sizeof(cmd), "lpadmin -x %s", prt_name);
	done = sys_run_cmd_no_output(cmd, "del-printer");
	if (done == FALSE) {
		grm_log_error("%s() : can't run  : %s", __FUNCTION__, cmd);
	}
	else {
		gboolean res;
		res = _existing_printer(prt_name, FALSE, TRUE);
		if (res == TRUE) {
			grm_log_debug("%s() : deleted cmd but exiting ppd  : %s", __FUNCTION__, prt_name);
			done = FALSE;
		}
		else {
			g_snprintf(cmd, sizeof(cmd), "/usr/bin/grac-apply.sh printer.%s disallow cups printer %s", prt_name, prt_name);
			res = sys_run_cmd_no_output(cmd, "del-printer");
			if (res == FALSE)
				grm_log_debug("%s() : can't run  : %s", __FUNCTION__, cmd);
			else
				grm_log_debug("%s() : deleted printer  : %s", __FUNCTION__, prt_name);
		}
	}
	return done;
}


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

//static
void* avahi_thread(void *data)
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

	grm_log_debug("start avahi_thread : pid = %d", getpid());

	fp = sys_popen(cmd, "r", &pid);
	if (fp == NULL) {
		grm_log_debug("%s() : can't run - %s", __FUNCTION__, cmd);
		return NULL;
	}
	grm_log_debug("avahi_thread : create child process = %d", pid);

	/*
	int fd = fileno(fp);
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		grm_log_debug("%s() : fcntl : not set NONBLOCK: %s", __FUNCTION__, strerror(errno));
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

			grm_log_debug("%s", buf);

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
				grm_log_error("%s() : inavlid printer information : %s", __FUNCTION__, rp);
			}
			else {
				c_strcpy(prt_name, ptr+1, sizeof(prt_name));
				grm_log_debug("%s() : catch new printer : %s", __FUNCTION__, rp);
				_delete_printer(prt_name);
				_delete_current_printer_list();
			}
	}
	grm_log_debug("out of avahi thread");

	if (ctrl->p_fp) {
		fp = ctrl->p_fp;
		pid = ctrl->p_pid;
		ctrl->p_fp = NULL;
		ctrl->p_pid = 0;

		sys_pclose(fp, pid);
	}

	ctrl->on_avahi_thread = FALSE;

	grm_log_debug("stop avahi thread : pid = %d tid=%d", sys_getpid(), sys_gettid());

	return NULL;
}


static gboolean _start_printer_monitor()
{
	c_memset(&G_AvahiCtrl, 0, sizeof(G_AvahiCtrl));

	/*
	int	res;
	pthread_t thread;

	res = pthread_create(&thread, NULL, avahi_thread, (void*)&G_AvahiCtrl);
	if (res != 0) {
		grm_log_error("can't create avahi_thread : %d", res);
		return FALSE;
	}
*/
	_start_watch_ppd();

	return TRUE;
}

static void _stop_printer_monitor()
{
	_stop_watch_ppd();
/*
	G_AvahiCtrl.normal_stop = TRUE;
	G_AvahiCtrl.request_stop = TRUE;

	if (G_AvahiCtrl.p_fp) {
		FILE *fp = G_AvahiCtrl.p_fp;
		int	pid = G_AvahiCtrl.p_pid;
		G_AvahiCtrl.p_fp = NULL;
		G_AvahiCtrl.p_pid = 0;

		sys_pclose(fp, pid);
	}

	while (G_AvahiCtrl.on_avahi_thread) {
		g_usleep(10*1000);
	}
*/
}


// return : -1 error  0:ignore  1:OK
static int _save_printer(char *prt_name)
{
	char *save_path = "/etc/gooroom/grac.d/printers";
	char	save_ppd[1024];
	char	cups_ppd[1024];
	struct stat st_save;
	struct stat st_cups;
	int		res;

	g_snprintf(save_ppd, sizeof(save_ppd), "%s/%s.ppd", save_path, prt_name);
	g_snprintf(cups_ppd, sizeof(cups_ppd), "/etc/cups/ppd/%s.ppd", prt_name);

	res = stat(cups_ppd, &st_cups);
	if (res != 0) {
		grm_log_debug("%s() : no ppd : %s : %s", __FUNCTION__, cups_ppd, strerror(errno));
		return 0;
	}

	res = stat(save_ppd, &st_save);
	if (res == 0) {
		if ((st_save.st_mtim.tv_sec > st_cups.st_mtim.tv_sec) ||
	      (st_save.st_mtim.tv_sec == st_cups.st_mtim.tv_sec &&
				 st_save.st_mtim.tv_nsec >= st_cups.st_mtim.tv_nsec) )
		{
			grm_log_debug("%s() : no need to save printer : %s", __FUNCTION__, prt_name);
			return 1;
		}
	}
	else {
		if (errno != ENOENT) {
			grm_log_error("%s() : %s : %s", __FUNCTION__, save_ppd, strerror(errno));
			return -1;
		}
	}

	char	cmd[1024];
	gboolean done;

	g_snprintf(cmd, sizeof(cmd), "cp %s %s", cups_ppd, save_ppd);
	done = sys_run_cmd_no_output(cmd, "save-ppd");
	if (done == FALSE) {
		grm_log_error("%s() : can't run  : %s", __FUNCTION__, cmd);
		return -1;
	}
	else {
		grm_log_debug("%s() : save printer : %s", __FUNCTION__, prt_name);
		return 1;
	}
}

static gboolean _recover_printer(char *prt_name)
{
	gboolean done = TRUE;
	char *save_path = "/etc/gooroom/grac.d/printers";
	char	save_ppd[1024];
	char	cups_ppd[1024];
	struct stat st_save;
	struct stat st_cups;
	int		res;

	g_snprintf(save_ppd, sizeof(save_ppd), "%s/%s.ppd", save_path, prt_name);
	g_snprintf(cups_ppd, sizeof(cups_ppd), "/etc/cups/ppd/%s.ppd", prt_name);

	res = stat(save_ppd, &st_save);
	if (res != 0) {
		grm_log_error("%s() : %s : %s", __FUNCTION__, save_ppd, strerror(errno));
		return FALSE;
	}

	res = stat(cups_ppd, &st_cups);
	if (res == 0) {
		if ((st_cups.st_mtim.tv_sec > st_save.st_mtim.tv_sec) ||
		   (st_cups.st_mtim.tv_sec == st_save.st_mtim.tv_sec &&
				st_cups.st_mtim.tv_nsec >= st_save.st_mtim.tv_nsec) )
		{
			grm_log_debug("%s() : no need to recover printer : %s", __FUNCTION__, prt_name);
			return TRUE;
		}
	}
	else {
		if (errno != ENOENT) {
			grm_log_error("%s() : %s : %s", __FUNCTION__, cups_ppd, strerror(errno));
			return FALSE;
		}
	}

	char	cmd[1024];
	g_snprintf(cmd, sizeof(cmd), "lpadmin -p %s -P %s", prt_name, save_ppd);
	done = sys_run_cmd_no_output(cmd, "recover-ppd");
	if (done == FALSE) {
		grm_log_error("%s() : can't run  : %s", __FUNCTION__, cmd);
	}
	else {
		grm_log_debug("%s() : recover printer : %s", __FUNCTION__, prt_name);
	}

	return done;
}


static gboolean _save_current_printer_list()
{
	gboolean done = TRUE;
	cups_dest_t *printer_dests = NULL;
	int printer_count = 0;

	printer_count = cupsGetDests(&printer_dests);
	if (printer_count == 0) {
			grm_log_debug("%s() : No printer to save", __FUNCTION__);
			return TRUE;
	}

	char *dir_path = "/etc/gooroom/grac.d/printers";
	char *file_prt_list = "printer-list";
	char	path_prt_list[1024];
	FILE *fp;

	mkdir(dir_path, 0755);

	g_snprintf(path_prt_list, sizeof(path_prt_list), "%s/%s", dir_path, file_prt_list);
	fp = g_fopen(path_prt_list, "w");
	if (fp == NULL) {
		grm_log_error("%s() : can't create : %s",__FUNCTION__,  path_prt_list);
		done = FALSE;
	}

	if (fp != NULL) {
		int idx, res;
		char *prt_name;
		for (idx=0; idx < printer_count; idx++) {
			prt_name = printer_dests[idx].name;
			res = _save_printer(prt_name);
			if (res < 0)
				done = FALSE;
			else if (res > 0)
				fprintf(fp, "%s\n", prt_name);
		}
		fclose(fp);
	}

	cupsFreeDests(printer_count, printer_dests);

	return done;
}

static gboolean _recover_saved_printer_list()
{
	gboolean done = TRUE;

	char *dir_path = "/etc/gooroom/grac.d/printers";
	char *file_prt_list = "printer-list";
	char	path_prt_list[1024];
	FILE *fp;

	g_snprintf(path_prt_list, sizeof(path_prt_list), "%s/%s", dir_path, file_prt_list);
	fp = g_fopen(path_prt_list, "r");
	if (fp == NULL) {
		grm_log_debug("no recover printer\n");
		return TRUE;
	}

	char prt_name[256];
	while (1) {
		if (fgets(prt_name, sizeof(prt_name), fp) == NULL)
			break;

		c_strtrim(prt_name, sizeof(prt_name));
		done &= _recover_printer(prt_name);
	}
	fclose(fp);

	unlink(path_prt_list);

	return done;
}

static gboolean _delete_current_printer_list()
{
	gboolean done = FALSE;
	cups_dest_t *printer_dests = NULL;
	int printer_count = 0;
	int	loop = 10;
	char cmd[256], buf[1024];
	gboolean res;
	gboolean again = FALSE;

	c_strcpy(cmd, "ls /etc/cups/ppd/*.ppd", sizeof(cmd));;

	while (loop > 0 && done == FALSE) {
		printer_count = cupsGetDests(&printer_dests);
		if (printer_count == 0) {
			grm_log_debug("%s() : there is no printer", __FUNCTION__);
			done = TRUE;
		}
		int idx;
		for (idx=0; idx < printer_count; idx++) {
			grm_log_debug("%s() : %d : delete printer : %s", __FUNCTION__, idx, printer_dests[idx].name);
			done &= _delete_printer(printer_dests[idx].name);
		}
		cupsFreeDests(printer_count, printer_dests);

		if (done == TRUE) {
			res = sys_run_cmd_get_output(cmd, "grac_printer", buf, sizeof(buf));
			if (res == TRUE) {
				grm_log_debug("%s() : check cups/ppd : [%s]", __FUNCTION__, buf);
				if (buf[0] != 0)
					done = FALSE;
			}
		}

		if (done == FALSE) {
			again = FALSE;
			loop--;
		}
		else {
			if (again == TRUE) {
				break;
			}
			else {
				again = TRUE;
				loop++;
			}
		}
		g_usleep(100*1000);
	}

	return done;
}



gboolean grac_printer_init()
{
	gboolean done = TRUE;

	done &= _recover_saved_printer_list();
	done &= _save_current_printer_list();

	return done;
}

gboolean grac_printer_apply(gboolean allow)
{
	gboolean done = TRUE;

	_stop_printer_monitor();
	done &= _recover_saved_printer_list();
	if (allow == FALSE) {
		done &= _save_current_printer_list();
		done &= _delete_current_printer_list();
		done &= _start_printer_monitor();
	}

	return done;
}

gboolean grac_printer_end()
{
	gboolean done = TRUE;

	_stop_printer_monitor();
	done &= _recover_saved_printer_list();

	return done;
}

