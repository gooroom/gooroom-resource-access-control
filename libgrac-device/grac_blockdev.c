/*
 * Copyright (c) 2015 - 2017 gooroom <gooroom@gooroom.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * grac_blockdev.c
 *
 *  Created on: 2017. 10. 30.
 *      Author: gooroom@gooroom.kr
 */

/**
  @file 	 	grac_blockdev.c
  @brief		block device ( libudev, udisksctl 이용)
  @details	\n
  				헤더파일 :  	grac_blockdev.h	\n
  				라이브러리:	libgrac-device.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libudev.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "grac_blockdev.h"
#include "grac_log.h"
#include "cutility.h"
#include "sys_file.h"
#include "sys_etc.h"

/**********************************************************************
    get udev data
**********************************************************************/

gboolean grac_udev_check_existing(char *sysname)
{
	gboolean done = FALSE;
	struct udev *udev;
	struct udev_device  *udev_dev;

	udev = udev_new();
	if (udev == NULL) {
		grac_log_debug("%s() : Can't create udev", __FUNCTION__);
		return FALSE;
	}

	udev_dev = udev_device_new_from_subsystem_sysname(udev, "block", sysname);
	if (udev_dev) {
		done = TRUE;
		udev_device_unref(udev_dev);
	}

	udev_unref(udev);

	return done;
}

gboolean grac_udev_get_type(char *sysname, char *type, int type_size)
{
	gboolean done = TRUE;
	struct udev *udev;
	struct udev_device  *udev_dev;

	type[0] = 0;

	udev = udev_new();
	if (udev == NULL) {
		grac_log_debug("%s() : Can't create udev", __FUNCTION__);
		return FALSE;
	}

	udev_dev = udev_device_new_from_subsystem_sysname(udev, "block", sysname);
	if (udev_dev) {
		const char *ptr;
		ptr = udev_device_get_devtype(udev_dev);
		if (ptr == NULL) {
			grac_log_debug("%s() : Can't get device type : %s", __FUNCTION__, sysname);
			done = FALSE;
		} else {
			c_strcpy(type, (char*)ptr, type_size);
		}
		udev_device_unref(udev_dev);
	} else {
		grac_log_debug("%s() : Can't get device info : %s", __FUNCTION__, sysname);
		done = FALSE;
	}

	udev_unref(udev);

	return done;
}

gboolean grac_udev_get_node(char *sysname, char *node, int node_size)
{
	gboolean done = TRUE;
	struct udev *udev;
	struct udev_device  *udev_dev;

	node[0] = 0;

	udev = udev_new();
	if (udev == NULL) {
		grac_log_debug("%s() : Can't create udev", __FUNCTION__);
		return FALSE;
	}

	udev_dev = udev_device_new_from_subsystem_sysname(udev, "block", sysname);
	if (udev_dev) {
		const char *ptr;
		ptr = udev_device_get_devnode(udev_dev);
		if (ptr == NULL) {
			grac_log_debug("%s() : Can't get device node : %s", __FUNCTION__, sysname);
			done = FALSE;
		} else {
			c_strcpy(node, (char*)ptr, node_size);
		}
		udev_device_unref(udev_dev);
	} else {
		grac_log_debug("%s() : Can't get device info : %s", __FUNCTION__, sysname);
		done = FALSE;
	}

	udev_unref(udev);

	return done;
}

// return -1:error o:false 1:true
int	grac_udev_get_removable(char *sysname)
{
	int	removable	= -1;
	struct udev *udev;
	struct udev_device  *udev_dev;

	udev = udev_new();
	if (udev == NULL) {
		grac_log_debug("%s() : Can't create udev", __FUNCTION__);
		return -1;
	}

	udev_dev = udev_device_new_from_subsystem_sysname(udev, "block", sysname);
	if (udev_dev) {
		const char *ptr;
		ptr = udev_device_get_sysattr_value(udev_dev, "removable");
		if (ptr == NULL) {
			grac_log_debug("%s() : not defined attr{removable} : %s", __FUNCTION__, sysname);
		} else {
			if (c_strmatch(ptr, "1"))
				removable = 1;
			else
				removable = 0;
		}
		udev_device_unref(udev_dev);
	} else {
		grac_log_debug("%s() : Can't get device info : %s", __FUNCTION__, sysname);
	}

	udev_unref(udev);

	return removable;
}

gboolean grac_udev_get_parent_sysname(char *sysname, char *parent, int parent_size)
{
	gboolean done = TRUE;
	struct udev *udev;
	struct udev_device  *udev_dev;

	*parent = 0;

	udev = udev_new();
	if (udev == NULL) {
		grac_log_debug("%s() : Can't create udev", __FUNCTION__);
		return FALSE;
	}

	udev_dev = udev_device_new_from_subsystem_sysname(udev, "block", sysname);
	if (udev_dev) {
		struct udev_device *p_dev = udev_device_get_parent(udev_dev);
		if (p_dev == NULL) {
			grac_log_debug("%s() : Can't get parent device : %s", __FUNCTION__, sysname);
			done = FALSE;
		} else {
			const char *ptr;
			ptr = udev_device_get_sysname(p_dev);
			if (ptr == NULL) {
				grac_log_debug("%s() : Can't get parent sysname : %s", __FUNCTION__, sysname);
				done = FALSE;
			} else {
				c_strcpy(parent, (char*)ptr, parent_size);
			}
		}
		udev_device_unref(udev_dev);
	} else {
		grac_log_debug("%s() : Can't get device info : %s", __FUNCTION__, sysname);
		done = FALSE;
	}

	udev_unref(udev);

	return done;
}

// return -1 : or count
int	grac_udev_get_children_sysnames(char *sysname, char *children, int children_size)
{
	int	count = 0;
	struct udev *udev;
	struct udev_enumerate  *udev_enum;
	struct udev_list_entry *dev_list, *dev_entry;
	struct udev_device  *udev_dev;
	struct udev_device  *parent_dev;
	const char *parent_name;

	*children = 0;

	udev = udev_new();
	if (udev == NULL) {
		grac_log_debug("%s() : Can't create udev", __FUNCTION__);
		return -1;
	}

	udev_dev = udev_device_new_from_subsystem_sysname(udev, "block", sysname);
	if (udev_dev == NULL) {
		grac_log_debug("%s() : Can't get device info : %s", __FUNCTION__, sysname);
		udev_unref(udev);
		return -1;
	}

	parent_dev = udev_device_get_parent(udev_dev);
	if (parent_dev == NULL) {
		grac_log_debug("%s() : Can't get device info : %s", __FUNCTION__, sysname);
		udev_device_unref(udev_dev);
		udev_unref(udev);
		return -1;
	}
	parent_name = udev_device_get_sysname(parent_dev);

	udev_enum = udev_enumerate_new(udev);
	if (udev == NULL) {
		grac_log_debug("%s() : Can't create udev_enumerate", __FUNCTION__);
		udev_device_unref(udev_dev);
		udev_unref(udev);
		return -1;
	}

	udev_enumerate_add_match_subsystem(udev_enum, "block");
	udev_enumerate_add_match_parent(udev_enum, parent_dev);

	udev_enumerate_scan_devices(udev_enum);

	dev_list = udev_enumerate_get_list_entry(udev_enum);
	udev_list_entry_foreach(dev_entry, dev_list) {
		struct udev_device  *ch_dev;
		const char	*name;
		name = udev_list_entry_get_name(dev_entry);
		ch_dev = udev_device_new_from_syspath(udev, name);
		if (ch_dev) {
			const char	*ch_name;
			ch_name = udev_device_get_sysname(ch_dev);
			if (ch_name && !c_strmatch(ch_name, parent_name)) {
				if (count > 0)
					c_strcat(children, ";", children_size);
				c_strcat(children, (char*)ch_name, children_size);
				count++;
			}
			udev_device_unref(ch_dev);
		}
	}

	udev_enumerate_unref(udev_enum);
	udev_device_unref(udev_dev);
	udev_unref(udev);

	return count;
}

// if parent, "lsblk" get all children's point
gboolean grac_udev_get_mount_point(char *sysname, char *mount, int mount_size)
{
	gboolean done;
	struct udev *udev;
	struct udev_device  *udev_dev;
	char	dev_type[32];

	*mount = 0;

	done = grac_udev_get_type(sysname, dev_type, sizeof(dev_type));
	if (done == FALSE)
		return FALSE;

	if (c_strmatch(dev_type, "disk")) {
		*mount = 0;
		return TRUE;
	}

	udev = udev_new();
	if (udev == NULL) {
		grac_log_debug("%s() : Can't create udev", __FUNCTION__);
		return FALSE;
	}

	udev_dev = udev_device_new_from_subsystem_sysname(udev, "block", sysname);
	if (udev_dev) {
		const char *node = udev_device_get_devnode(udev_dev);
		if (node == NULL) {
			grac_log_debug("%s() : Can't get device node", __FUNCTION__, sysname);
			done = FALSE;
		} else {
			gboolean res;
			char cmd[1024], buf[1024];

			g_snprintf(cmd, sizeof(cmd), "/bin/lsblk -o MOUNTPOINT -n %s", node);
			res = sys_run_cmd_get_output(cmd, (char*)__FUNCTION__, buf, sizeof(buf));
			if (res) {
				c_strtrim(buf, sizeof(buf));
				c_strcpy(mount, buf, mount_size);
			} else {
				grac_log_debug("%s() : Can't get mount position", __FUNCTION__, sysname);
				done = FALSE;
			}
		}
		udev_device_unref(udev_dev);
	} else {
		grac_log_debug("%s() : Can't get device info : %s", __FUNCTION__, sysname);
		done = FALSE;
	}

	udev_unref(udev);

	return done;
}

/**********************************************************************
    eject process
**********************************************************************/

static void _wait_udev_finish(int usec)
{
	struct udev *udev;
	struct udev_device *udev_dev;
	struct udev_monitor *monitor;
	int	fd;

	grac_log_debug("%s() : start", __FUNCTION__);

	udev = udev_new();
	if (udev == NULL) {
		grac_log_debug("%s() : Can't create udev", __FUNCTION__);
		return;
	}

	monitor = udev_monitor_new_from_netlink(udev, "udev");
	if (monitor == NULL) {
		grac_log_debug("%s() : Can't create monitor", __FUNCTION__);
		udev_unref(udev);
		return;
	}

	udev_monitor_filter_add_match_subsystem_devtype(monitor, "usb", NULL);
	udev_monitor_filter_add_match_subsystem_devtype(monitor, "block", NULL);
	udev_monitor_filter_add_match_subsystem_devtype(monitor, "bdi", NULL);

	udev_monitor_enable_receiving(monitor);

	fd = udev_monitor_get_fd(monitor);

	while (1) {
		fd_set fds;
		struct timeval tv;
		int	ret;

		c_memset(&tv, 0, sizeof(tv));
		tv.tv_sec = usec / (1000*1000);
		tv.tv_usec = usec % (1000*1000);

		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		ret = select(fd+1, &fds, NULL, NULL, &tv);
		if (ret > 0) {
			if (FD_ISSET(fd, &fds)) {
				udev_dev = udev_monitor_receive_device(monitor);
				if (udev_dev) {
					grac_log_debug("%s() : get device info", __FUNCTION__, udev_device_get_devnode(udev_dev));
					udev_device_unref(udev_dev);
				} else {
					grac_log_debug("%s() : not received the  device info", __FUNCTION__);
				}
			}
		} else if (ret == 0) {
			break;
		} else {
			grac_log_debug("%s() : select error : %s", __FUNCTION__, strerror(errno));
			break;
		}
	}

	udev_monitor_unref(monitor);
	udev_unref(udev);

	grac_log_debug("%s() : end", __FUNCTION__);
}

static gboolean _do_unmount_children(char *sysname)
{
	gboolean done = FALSE;
	int		idx;
	char	children[1024];
	gboolean res;
	gboolean need_unmount;
	int	  retry, retry_max = 10;
	int		delay = 1000*1000;
	char	cmd[1024];

	for (retry=0; retry < retry_max; retry++)  {
		need_unmount = FALSE;
		int count = grac_udev_get_children_sysnames(sysname, children, sizeof(children));
		char* ch_name = children;
		for (idx=0; idx < count; idx++) {
			char	*ptr;
			ptr = c_strchr(ch_name, ';', sizeof(children));
			if (ptr) {
				*ptr = 0;
				ptr++;
			}
			if (ch_name[0] != 0) {
				char	mount[1024];
				res = grac_udev_get_mount_point(ch_name, mount, sizeof(mount));
				if (res && mount[0]) { 			// unmount
					char	ch_node[256];
					grac_udev_get_node(ch_name, ch_node, sizeof(ch_node));
					g_snprintf(cmd, sizeof(cmd), "/usr/bin/udisksctl unmount -b %s", ch_node);
					sys_run_cmd_no_output(cmd, (char*)__FUNCTION__);
					res = grac_udev_get_mount_point(ch_name, mount, sizeof(mount));
					if (res && mount[0])  			// check result
						need_unmount = TRUE;
				}
			}
			ch_name = ptr;
		} // end of loop for child

		if (need_unmount == FALSE) {
			done = TRUE;
			break;
		}
		_wait_udev_finish(delay);
	}

	if (need_unmount == TRUE) {
		grac_log_error("%s() : unmount error : %s", __FUNCTION__, sysname);
	}

	return done;
}


static gboolean _do_eject(char *sysname)
{
	gboolean done = FALSE;
	char 	dev_type[32];
	char 	dev_node[256];
	char	cmd[1024];
	int 	removable;
	int	  retry, retry_max = 10;
	int		delay = 1000*1000;
	gboolean res;

	res = grac_udev_get_type(sysname, dev_type, sizeof(dev_type));
	if (res == FALSE) {
		return FALSE;
	}
	res = grac_udev_get_node(sysname, dev_node, sizeof(dev_node));
	if (res == FALSE) {
		return FALSE;
	}

	if (c_strmatch(dev_type, "partition")) {
		grac_log_debug("%s() : do not call for partition", __FUNCTION__);
		return FALSE;
	}

	removable = grac_udev_get_removable(sysname);
	if (removable == 1) {
		for (retry=0; retry < retry_max; retry++)  {
			g_snprintf(cmd, sizeof(cmd), "/usr/bin/eject %s", dev_node);
			sys_run_cmd_no_output(cmd, (char*)__FUNCTION__);
			res = grac_udev_check_existing(sysname);
			if (res == FALSE) {
				done = TRUE;
				break;
			}

			g_snprintf(cmd, sizeof(cmd), "/usr/bin/udisksctl power-off -b %s", dev_node);
			sys_run_cmd_no_output(cmd, (char*)__FUNCTION__);
			res = grac_udev_check_existing(sysname);
			if (res == FALSE) {
				done = TRUE;
				break;
			}

			_wait_udev_finish(delay);
		}
		if (retry >= retry_max) {
			grac_log_error("%s() : eject error : %s", __FUNCTION__, dev_node);
		}
	} else {
		g_snprintf(cmd, sizeof(cmd), "/usr/bin/udisksctl power-off -b %s", dev_node);
		for (retry=0; retry < retry_max; retry++)  {
			res = _do_unmount_children(sysname);
			if (res == TRUE) {
				sys_run_cmd_no_output(cmd, (char*)__FUNCTION__);
				res = grac_udev_check_existing(sysname);
				if (res == FALSE) {
					done = TRUE;
					break;
				}
			}
			_wait_udev_finish(delay);
		}
		if (retry >= retry_max) {
			grac_log_error("%s() : power-off error : %s", __FUNCTION__, dev_node);
		}
	}

	return done;
}

// sysname : sda sda1 ...
gboolean grac_eject(char *p_sysname)
{
	gboolean done = TRUE;
	char dev_type[32];
	char *sysname;

	sysname = c_strrchr(p_sysname, '/', 1024);
	if (sysname)
		sysname++;
	else
		sysname = p_sysname;

	grac_log_debug("%s() : start : %s", __FUNCTION__, sysname);

	int n = 10;  // only for test -------------------------------------
	while (n > 0) {
		done = grac_udev_get_type(sysname, dev_type, sizeof(dev_type));
		if (done)
			break;
		g_usleep(1000*1000);
		n--;
	}

	done = grac_udev_get_type(sysname, dev_type, sizeof(dev_type));
	if (done == FALSE) {
		grac_log_debug("%s() : stop by error : %s", __FUNCTION__, sysname);
		return FALSE;
	}

	if (c_strmatch(dev_type, "partition")) {
		char	parent[256];
		done = grac_udev_get_parent_sysname(sysname, parent, sizeof(parent));
		if (done == FALSE) {
			grac_log_debug("%s() : stop by error : %s", __FUNCTION__, sysname);
			return FALSE;
		}
		done = _do_eject(parent);
	} else {
		done = _do_eject(sysname);
	}

	grac_log_debug("%s() : end : %s : result = %d", __FUNCTION__, sysname, (int)done);

	return done;
}

/**********************************************************************
    blokc device monitor and apply rule
**********************************************************************/

struct _BlockCtrl {
	gboolean	request_stop;
	gboolean	normal_stop;
	gboolean	on_thread;
	FILE	*p_fp;
	pid_t  p_pid;
	int		perm;
	pthread_t th;
} BlockCtrl = { 0 };

#define P_DENY      0
#define P_READONLY  1
#define P_READWRITE 2

#define MONITOR_BLOCK_DIR			"/etc/gooroom/grac.d/blocks"
#define MONITOR_BLOCK_PATH		"/etc/gooroom/grac.d/blocks/block-list"

static gboolean _init_data()
{
	gboolean done = TRUE;

	if (sys_file_is_existing(MONITOR_BLOCK_DIR) == FALSE)
		mkdir(MONITOR_BLOCK_DIR, 0755);

	FILE *fp = g_fopen(MONITOR_BLOCK_PATH, "w");
	if (fp) {
		fclose(fp);
	} else {
		done = FALSE;
		grac_log_error("grac_block_devCan't create : %s", MONITOR_BLOCK_PATH);
	}

	return done;
}

static void _free_data()
{
}

static void* block_thread(void *data)
{
	struct _BlockCtrl *ctrl = (struct _BlockCtrl*)data;
	char	buf[1024];
	FILE	*fp;
	pid_t	pid;

	char	*cmd = "/usr/bin/tail -f " MONITOR_BLOCK_PATH;

	grac_log_debug("start block_thread : pid = %d", getpid());

	fp = sys_popen(cmd, "r", &pid);
	if (fp == NULL) {
		grac_log_debug("%s() : can't run - %s", __FUNCTION__, cmd);
		return NULL;
	}
	grac_log_debug("block_thread : create child process = %d", pid);

	ctrl->p_fp = fp;
	ctrl->p_pid = pid;
	ctrl->on_thread = TRUE;
	ctrl->request_stop = FALSE;

	while (ctrl->request_stop == FALSE) {

			if (fgets(buf, sizeof(buf), fp) == NULL) {
				g_usleep(100*1000);
				continue;
			}
			grac_log_debug("block_thread : %s", buf);

			c_strtrim(buf, sizeof(buf));

			if (ctrl->perm == P_DENY)
				grac_eject(buf);
	}

	grac_log_debug("out of block_thread");

	if (ctrl->p_fp) {
		fp = ctrl->p_fp;
		pid = ctrl->p_pid;
		ctrl->p_fp = NULL;
		ctrl->p_pid = 0;

		sys_pclose(fp, pid);
	}

	ctrl->on_thread = FALSE;

	grac_log_debug("stop block_thread : pid = %d tid=%d", sys_getpid(), sys_gettid());

	return NULL;
}


static gboolean _start_block_dev_monitor(int perm)
{
	int	res;
	pthread_t thread;

	c_memset(&BlockCtrl, 0, sizeof(BlockCtrl));
	BlockCtrl.perm = perm;

	res = pthread_create(&thread, NULL, block_thread, (void*)&BlockCtrl);
	if (res != 0) {
		grac_log_error("can't create avahi_thread : %d", res);
		return FALSE;
	}

	BlockCtrl.th = thread;

	return TRUE;
}

static void _stop_block_dev_monitor()
{
	BlockCtrl.normal_stop = TRUE;
	BlockCtrl.request_stop = TRUE;

	grac_log_debug("%s() : start block thread", __FUNCTION__);

	if (BlockCtrl.p_fp) {
		FILE *fp = BlockCtrl.p_fp;
		int	pid = BlockCtrl.p_pid;
		BlockCtrl.p_fp = NULL;
		BlockCtrl.p_pid = 0;

		sys_pclose(fp, pid);
	}

	// wait for maximum 1 seconds
	int cnt = 10;
	while (cnt > 0) {
		if (BlockCtrl.on_thread == FALSE)
			break;
		g_usleep(100*1000);
		cnt--;
	}

	pthread_cancel(BlockCtrl.th);

	if (BlockCtrl.on_thread == FALSE)
		grac_log_debug("%s() : end block thread", __FUNCTION__);
	else
		grac_log_debug("%s() : end : can't stop block thread", __FUNCTION__);
}


gboolean grac_blockdev_init()
{
	gboolean done;

	done = _init_data();

	return done;
}

gboolean grac_blockdev_apply_disallow()
{
	gboolean done;

	done = _init_data();
	_stop_block_dev_monitor();

	done &= _start_block_dev_monitor(P_DENY);

	return done;
}

gboolean grac_blockdev_apply_readonly()
{
	gboolean done;

	done = _init_data();
	_stop_block_dev_monitor();

	done &= _start_block_dev_monitor(P_READONLY);

	return done;
}

gboolean grac_blockdev_apply_normal()
{
	gboolean done;

	done = _init_data();
	_stop_block_dev_monitor();

	return done;
}

gboolean grac_blockdev_end()
{
	gboolean done = TRUE;

	_stop_block_dev_monitor();
	_free_data();

	return done;
}


