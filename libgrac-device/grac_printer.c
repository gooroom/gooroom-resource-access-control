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
#include "grac_log.h"
#include "cutility.h"
#include "sys_file.h"
#include "sys_etc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cups/cups.h>
#include <errno.h>

#include <sys/inotify.h>

#include <glib.h>
#include <glib/gstdio.h>

struct _WatchCtrl {
	gboolean	stop;
	gboolean	on_thread;

	char			target[1024];
	gboolean	checked;
	struct _WatchCtrl *pair;
	gboolean ppd;
	pthread_t th;

	int		i_fd;
	int		i_wd;
};

struct _PrtCtrl {
	gboolean	request_stop;
	gboolean	normal_stop;
	gboolean	on_avahi_thread;
	FILE	*p_fp;
	pid_t  p_pid;

	struct _WatchCtrl ppd_watch;
	struct _WatchCtrl conf_watch;
};

static struct _PrtCtrl  PrtCtrl = { 0 };

static gboolean _delete_current_printer_list(gboolean fromThread);


G_LOCK_DEFINE_STATIC (watch_thread_lock);

static void* _watch_thread(void *data)
{
	struct _WatchCtrl *ctrl = (struct _WatchCtrl*)data;
	int	fd, wd;

	fd = inotify_init();
	if (fd <= 0) {
		grac_log_error("%s() : can't init notify : %s", __FUNCTION__, ctrl->target);
		return NULL;
	}

	wd = inotify_add_watch(fd, ctrl->target, IN_CLOSE_WRITE);
	if (wd <= 0) {
		grac_log_error("%s() : can't add watch : %s : %s", __FUNCTION__, ctrl->target, c_strerror(-1));
		sys_file_close(&fd);
		return NULL;
	}

	ctrl->on_thread = TRUE;

	ctrl->i_fd = fd;
	ctrl->i_wd = wd;

	int n;
	char	buf[1024];

	grac_log_debug("%s() : start of watching %s, pid=%d, tid=%d", __FUNCTION__,  ctrl->target, sys_getpid(), sys_gettid());

	while (ctrl->stop == FALSE) {
		n = sys_file_read(fd, buf, sizeof(buf));
		if (n <= 0)
				break;

		if (ctrl->stop == TRUE)
			break;

		grac_log_debug("%s() : watched modify : %s", __FUNCTION__, ctrl->target);

		if (ctrl->ppd)
			_delete_current_printer_list(TRUE);

/*
		if (ctrl->pair) {
			if (ctrl->pair->checked) {
				ctrl->pair->checked = FALSE;
				_delete_current_printer_list(TRUE);
			}
			else {
				ctrl->checked = TRUE;
			}
		}
		else {
			_delete_current_printer_list(TRUE);
		}
*/
	}

	ctrl->on_thread = FALSE;

	G_LOCK(watch_thread_lock);
	fd = ctrl->i_fd;
	wd = ctrl->i_wd;
	ctrl->i_fd = -1;
	ctrl->i_wd = -1;
	G_UNLOCK(watch_thread_lock);

	if (fd > 0 && wd > 0) {
		inotify_rm_watch(fd, wd);
		sys_file_close(&fd);
	}

	grac_log_debug("%s() : end of watching %s, pid=%d, tid=%d", __FUNCTION__,  ctrl->target, sys_getpid(), sys_gettid());

	return NULL;
}

static gboolean _start_watch_thread(gboolean ppd)
{
	int	res;
	pthread_t thread;
	char	*target;
	struct _WatchCtrl *ctrl;

	if (ppd == TRUE) {
		target = "/etc/cups/ppd";
		if (sys_file_is_existing(target) == FALSE) {
			mkdir(target, 0755);
			sys_file_change_owner_group(target, "root", "lp");
		}
		ctrl = &PrtCtrl.ppd_watch;
		c_memset(ctrl, 0, sizeof(struct _WatchCtrl));
		ctrl->pair = &PrtCtrl.conf_watch;
	}
	else {
		target = "/etc/cups/printers.conf";
		if (sys_file_is_existing(target) == FALSE) {
			FILE *fp = g_fopen(target, "w");
			if (fp) {
				fclose(fp);
				g_chmod(target, 0600);
				sys_file_change_owner_group(target, "root", "lp");
			}
		}

		ctrl = &PrtCtrl.conf_watch;
		c_memset(ctrl, 0, sizeof(struct _WatchCtrl));
		ctrl->pair = &PrtCtrl.ppd_watch;
	}
	c_strcpy(ctrl->target, target, sizeof(ctrl->target));
	ctrl->ppd = ppd;

	if (sys_file_is_existing(target) == FALSE) {
		grac_log_error("%s(): target is not existed : %s", target);
		if (ctrl->pair)
			ctrl->pair->pair = NULL;
		return FALSE;
	}

	ctrl->i_fd = -1;
	ctrl->i_wd = -1;

	res = pthread_create(&thread, NULL, _watch_thread, (void*)ctrl);
	if (res != 0) {
		grac_log_error("can't create watch_thread : %s", ctrl->target);
		if (ctrl->pair)
			ctrl->pair->pair = NULL;
		return FALSE;
	}

	ctrl->th = thread;

	return TRUE;
}

static void _stop_watch_thread(gboolean ppd)
{

	struct _WatchCtrl *ctrl;

	if (ppd == TRUE) {
		ctrl = &PrtCtrl.ppd_watch;
	}
	else {
		ctrl = &PrtCtrl.conf_watch;
	}

	grac_log_debug("%s() : start : %s", __FUNCTION__,  ctrl->target);

	ctrl->checked = FALSE;
	ctrl->stop = TRUE;

	if (ctrl->i_fd > 0) {
		int fd, wd;
		G_LOCK(watch_thread_lock);
		fd = ctrl->i_fd;
		wd = ctrl->i_wd;
		ctrl->i_fd = -1;
		ctrl->i_wd = -1;
		G_UNLOCK(watch_thread_lock);

		inotify_rm_watch(fd, wd);
		sys_file_close(&fd);
	}

	int loop = 10;
	while (loop > 0) {
		if (ctrl->on_thread == FALSE)
			break;
		g_usleep(10*1000);
		loop--;
	}

	pthread_cancel(ctrl->th);

	if (ctrl->on_thread == FALSE)
		grac_log_debug("%s() : end : %s", __FUNCTION__,  ctrl->target);
	else
		grac_log_debug("%s() : end : can't stop : %s", __FUNCTION__,  ctrl->target);
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
				grac_log_debug("%s() : no cups dest but existing ppd", __FUNCTION__, prt_name);
				found = TRUE;
			}
		}
	}

	return found;
}

static gboolean _existing_any_printer(gboolean cups, gboolean ppd)
{
	gboolean found = FALSE;

	if (cups) {
		cups_dest_t *printer_dests = NULL;
		int printer_count = 0;
		printer_count = cupsGetDests(&printer_dests);
		if (printer_count > 0) {
			grac_log_debug("%s() : check cups api : count : %d", __FUNCTION__, printer_count);
			found = TRUE;
		}
		cupsFreeDests(printer_count, printer_dests);

		if (found == FALSE)
			return FALSE;
	}

	if (ppd) {
		found = FALSE;
		char	buf[256];
		char *cmd = "ls /etc/cups/ppd/*.ppd";
		gboolean res = sys_run_cmd_get_output(cmd, "grac_printer", buf, sizeof(buf));
		if (res == TRUE) {
			if (buf[0] != 0) {
				grac_log_debug("%s() : check cups/ppd : [%s]", __FUNCTION__, buf);
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
	int		loop;

	if (_existing_printer(prt_name, TRUE, TRUE) == FALSE) {
		grac_log_error("%s() : not existing printer", __FUNCTION__);
		return TRUE;
	}

	g_snprintf(cmd, sizeof(cmd), "lpadmin -x %s", prt_name);
	done = sys_run_cmd_no_output(cmd, "del-printer");
	if (done == FALSE) {
		grac_log_error("%s() : can't run  : %s", __FUNCTION__, cmd);
	}
	else {
		gboolean res;
		done = FALSE;
		loop = 10;
		while (loop > 0) {
			res = _existing_printer(prt_name, TRUE, FALSE);
			if (res == TRUE) {
				grac_log_debug("%s() : deleted cmd but exiting cups api : %s", __FUNCTION__, prt_name);
			}
			else {
				res = _existing_printer(prt_name, FALSE, TRUE);
				if (res == TRUE) {
					grac_log_debug("%s() : deleted cmd but exiting ppd  : %s", __FUNCTION__, prt_name);
				}
				else {
					g_snprintf(cmd, sizeof(cmd), "/usr/bin/grac-apply.sh printer.%s disallow cups printer %s", prt_name, prt_name);
					res = sys_run_cmd_no_output(cmd, "del-printer");
					if (res == FALSE)
						grac_log_debug("%s() : can't run  : %s", __FUNCTION__, cmd);
					else
						grac_log_debug("%s() : deleted printer  : %s", __FUNCTION__, prt_name);
					done = TRUE;
					break;
				}
			}
			g_usleep(100*1000);
			loop--;
		}
	}
	return done;
}

static gboolean _start_printer_monitor()
{
	c_memset(&PrtCtrl, 0, sizeof(PrtCtrl));

	_start_watch_thread(TRUE);
	//_start_watch_thread(FALSE);  only for debug

	return TRUE;
}

static void _stop_printer_monitor()
{
	//_stop_watch_thread(FALSE);  only for debug
	_stop_watch_thread(TRUE);
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
		grac_log_debug("%s() : no ppd : %s : %s", __FUNCTION__, cups_ppd, c_strerror(-1));
		return 0;
	}

	res = stat(save_ppd, &st_save);
	if (res == 0) {
		if ((st_save.st_mtim.tv_sec > st_cups.st_mtim.tv_sec) ||
	      (st_save.st_mtim.tv_sec == st_cups.st_mtim.tv_sec &&
				 st_save.st_mtim.tv_nsec >= st_cups.st_mtim.tv_nsec) )
		{
			grac_log_debug("%s() : no need to save printer : %s", __FUNCTION__, prt_name);
			return 1;
		}
	}
	else {
		if (errno != ENOENT) {
			grac_log_error("%s() : %s : %s", __FUNCTION__, save_ppd, c_strerror(-1));
			return -1;
		}
	}

	char	cmd[1024];
	gboolean done;

	g_snprintf(cmd, sizeof(cmd), "cp %s %s", cups_ppd, save_ppd);
	done = sys_run_cmd_no_output(cmd, "save-ppd");
	if (done == FALSE) {
		grac_log_error("%s() : can't run  : %s", __FUNCTION__, cmd);
		return -1;
	}
	else {
		grac_log_debug("%s() : save printer : %s", __FUNCTION__, prt_name);
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
		grac_log_error("%s() : %s : %s", __FUNCTION__, save_ppd, strerror(errno));
		return FALSE;
	}

	res = stat(cups_ppd, &st_cups);
	if (res == 0) {
		if ((st_cups.st_mtim.tv_sec > st_save.st_mtim.tv_sec) ||
		   (st_cups.st_mtim.tv_sec == st_save.st_mtim.tv_sec &&
				st_cups.st_mtim.tv_nsec >= st_save.st_mtim.tv_nsec) )
		{
			grac_log_debug("%s() : no need to recover printer : %s", __FUNCTION__, prt_name);
			return TRUE;
		}
	}
	else {
		if (errno != ENOENT) {
			grac_log_error("%s() : %s : %s", __FUNCTION__, cups_ppd, strerror(errno));
			return FALSE;
		}
	}

	char	cmd[1024];
	g_snprintf(cmd, sizeof(cmd), "lpadmin -p %s -P %s", prt_name, save_ppd);
	done = sys_run_cmd_no_output(cmd, "recover-ppd");
	if (done == FALSE) {
		grac_log_error("%s() : can't run  : %s", __FUNCTION__, cmd);
	}
	else {
		grac_log_debug("%s() : recover printer : %s", __FUNCTION__, prt_name);
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
			grac_log_debug("%s() : No printer to save", __FUNCTION__);
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
		grac_log_error("%s() : can't create : %s",__FUNCTION__,  path_prt_list);
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
		grac_log_debug("no recover printer\n");
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

static gboolean _delete_current_printer_list(gboolean fromThread)
{
	gboolean done = FALSE;
	cups_dest_t *printer_dests = NULL;
	int printer_count = 0;
	int	loop = 10;
	gboolean again = FALSE;

	grac_log_debug("%s() : start1", __FUNCTION__);

	// waiting adding printer
	if (fromThread) {
		loop = 10;
		while (loop > 0) {
			if (_existing_any_printer(TRUE, TRUE))
				break;
			g_usleep(200*1000);
			loop--;
		}
	}


	grac_log_debug("%s() : start2", __FUNCTION__);

	loop = 10;

	while (loop > 0 && done == FALSE) {
		printer_count = cupsGetDests(&printer_dests);
		if (printer_count == 0) {
			grac_log_debug("%s() : there is no printer", __FUNCTION__);
			done = TRUE;
		}
		int idx;
		for (idx=0; idx < printer_count; idx++) {
			grac_log_debug("%s() : %d : delete printer : %s", __FUNCTION__, idx, printer_dests[idx].name);
			done &= _delete_printer(printer_dests[idx].name);
		}
		cupsFreeDests(printer_count, printer_dests);

		if (done) {
			if (again == FALSE) {
				again = TRUE;
				grac_log_debug("%s() : OK retry check", __FUNCTION__);
			}
			else if (_existing_any_printer(TRUE, TRUE) == FALSE)
				break;
			g_usleep(500*1000);
		}
		else {
			g_usleep(100*1000);
			again = FALSE;
			loop--;
		}
	}

	grac_log_debug("%s() : end", __FUNCTION__);

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
		done &= _delete_current_printer_list(FALSE);
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

