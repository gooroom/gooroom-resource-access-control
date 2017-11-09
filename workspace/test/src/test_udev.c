/*
 * test_udev.c
 *
 *  Created on: 2017. 11. 3.
 *      Author: yang
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#include <libudev.h>

#include "grac_log.h"
#include "grac_config.h"
#include "cutility.h"
#include "sys_user.h"
#include "sys_etc.h"


gboolean test_udev()
{
	struct udev *udev;
	struct udev_enumerate  *udev_enum;
	struct udev_list_entry *dev_list, *dev_entry;
	struct udev_device  *udev_dev;

//	struct	udev_monitor *udev_mon;
//	int	fd;

	udev = udev_new();
	if (udev == NULL) {
		printf("Can't create udev\n");
		return FALSE;
	}

	udev_enum = udev_enumerate_new(udev);
	if (udev == NULL) {
		printf("Can't create udev_enumerate\n");
		udev_unref(udev);
		return FALSE;
	}

	udev_enumerate_add_match_subsystem(udev_enum, "block");

	udev_enumerate_scan_devices(udev_enum);

	dev_list = udev_enumerate_get_list_entry(udev_enum);
	udev_list_entry_foreach (dev_entry, dev_list) {
		const char	*name;
		name = udev_list_entry_get_name(dev_entry);
		// value = udev_list_entry_get_value(dev_entry);  // value is NULL
		printf("Name:%s\n", name);

		udev_dev = udev_device_new_from_syspath(udev, name);
		const char *node;
		node = udev_device_get_devnode(udev_dev);
		printf("Node:%s\n", node);

		const char *type;
		type = udev_device_get_devtype(udev_dev);
		printf("Type:%s\n", type);

		const char *sysname;
		sysname = udev_device_get_sysname(udev_dev);
		printf("SysName:%s\n", sysname);

		// properties
		struct udev_list_entry *prop_list, *prop_entry;
		prop_list = udev_device_get_properties_list_entry(udev_dev);
		udev_list_entry_foreach (prop_entry, prop_list) {
			const char	*prop, *value;
			prop = udev_list_entry_get_name(prop_entry);
			value = udev_list_entry_get_value(prop_entry);
			if (value == NULL)
				printf("P:%s <NotSet>\n", prop);
			else
				printf("P:%s:%s\n", prop, value);
		}

		// attributes
		struct udev_list_entry *attr_list, *attr_entry;
		attr_list = udev_device_get_sysattr_list_entry(udev_dev);
		udev_list_entry_foreach (attr_entry, attr_list) {
			const char	*attr, *value;
			attr = udev_list_entry_get_name(attr_entry);
			//value = udev_list_entry_get_value(attr_entry);	// always NULL
			value = udev_device_get_sysattr_value(udev_dev, attr);
			if (value == NULL)
				printf("A:%s <NotSet>\n", attr);
			else
				printf("A:%s:%s\n", attr, value);
		}
		printf("-- removable : %s\n", udev_device_get_sysattr_value(udev_dev, "removable"));
		printf("-- ro : %s\n", udev_device_get_sysattr_value(udev_dev, "ro"));

		char cmd[1024], buf[1024];;
		snprintf(cmd, sizeof(cmd), "/bin/lsblk -o MOUNTPOINT -n %s", node);
		sys_run_cmd_get_output(cmd, "test", buf, sizeof(buf));
		printf("Mount:%s\n", buf);

		struct udev_device *parent = udev_device_get_parent(udev_dev);
		node = udev_device_get_devnode(parent);
		printf("Parent Node:%s\n", node);

		printf("\n");
	}

	udev_enumerate_unref(udev_enum);
	udev_unref(udev);

	return TRUE;
}

typedef struct _BlockDev {
	char	name[256];
	char	path[1024];
	char	type[32];
	char	mount[1024];
	gboolean	removable;
	gboolean	readonly;
} BlockDev;


/*
	/bin/lsblk -n -P -p
	NAME="/dev/sr0"  MAJ:MIN="11:0"  RM="1" SIZE="968.4M" RO="0" TYPE="rom"  MOUNTPOINT=""
	NAME="/dev/vda"  MAJ:MIN="254:0" RM="0" SIZE="16G"    RO="0" TYPE="disk" MOUNTPOINT=""
	NAME="/dev/vda1" MAJ:MIN="254:1" RM="0" SIZE="15.3G"  RO="0" TYPE="part" MOUNTPOINT="/"
*/
static gboolean	_get_field(char *buf, int buf_size, char *key, char *data, int data_size)
{
	char	*ptr;
	int		idx;

	*data = 0;
	ptr = c_strstr(buf, key, buf_size);
	if (ptr == NULL) {
		return FALSE;
	}

	ptr++;
	idx = c_strchr_idx(ptr, '\"', buf_size);
	if (idx <= 0)
		return FALSE;

	if (idx >= data_size-1)
		return FALSE;

	c_memcpy(data, ptr, idx);
	data[idx] = 0;

	return TRUE;
}

static gboolean	_analize_line(char *buf, int buf_size, BlockDev* dev, int *next)
{
	gboolean done = TRUE;
	int idx;
	char *ptr;
	char tmp[256];

	idx = c_strchr_idx(buf, '\n', buf_size);
	if (idx >= 0)
		buf[idx] = 0;

	done &= _get_field(buf, buf_size, "NAME", dev->path, sizeof(dev->path));
	ptr = c_strrchr(dev->path, '\\', sizeof(dev->path));
	if (ptr)
		c_strcpy(dev->name, ptr, sizeof(dev->name));
	else
		c_strcpy(dev->name, dev->path, sizeof(dev->name));

	done &= _get_field(buf, buf_size, "TYPE", dev->type, sizeof(dev->type));
	done &= _get_field(buf, buf_size, "MOUNTPOINT", dev->mount, sizeof(dev->mount) );

	done &= _get_field(buf, buf_size, "RM", tmp, sizeof(tmp));
	if (c_strmatch(tmp, "1"))
		dev->removable = TRUE;
	else
		dev->removable = FALSE;
	done &= _get_field(buf, buf_size, "RO", tmp, sizeof(tmp));
	if (c_strmatch(tmp, "1"))
		dev->readonly = TRUE;
	else
		dev->readonly = FALSE;

	if (idx >= 0) {
		buf[idx] = '\n';
		*next = idx + 1;
	}
	else {
		*next = c_strlen(buf, buf_size);
	}

	return done;
}

void t_block()
{
	if (grac_blockdev_init() == FALSE) {
		printf("Init error\n");
		return;
	}

	char	buf[256];
	while (1) {
		if (fgets(buf, 256, stdin) == NULL)
			break;
		c_strtrim(buf, 256);
		if (c_strmatch(buf, "read_only")) {
			printf("allow\n");
			grac_blockdev_apply_readonly();
		}
		else if (c_strmatch(buf, "allow")) {
			printf("allow - no operation\n");
		}
		else if (c_strmatch(buf, "disallow")) {
			printf("disallow\n");
			grac_blockdev_apply_disallow();
		}
		else if (c_strmatch(buf, "exit")) {
			printf("exit\n");
			break;
		}
		else {
			printf("loop\n");
		}
	}

	if (grac_blockdev_end() == FALSE) {
		printf("Ending error\n");
		return;
	}
}
