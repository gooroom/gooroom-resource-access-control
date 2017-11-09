/*
 * grac_adjust_udev_rule.c
 *
 *  Created on: 2017. 9. 20.
 *      Author: user
 */

/**
  @file 	 	grac_adjust_udev_rule.c
  @brief		udev rule의 내용 중 기본적으로 사용중인 block device등이 제외시킨다.
  @details
  				헤더파일 :  	grac_adjust_udev_rule.h	\n
  				라이브러리:	libgrac-device.so
 */

#include "grac_adjust_udev_rule.h"
#include "grac_log.h"
#include "cutility.h"
#include "sys_etc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>

typedef struct _GracBlockDevice GracBlockDevice;
struct _GracBlockDevice {
	char 		*dev_name;
	char		*dev_type;			// "disk" "part" "rom"
	char		*mount_point;
//char		*sub_system;
	gboolean	readonly;
	gboolean	removable;
//gboolean	hotplug;

	gboolean	this_root;
	gboolean	child_root;

	GracBlockDevice	*children;
	GracBlockDevice	*parent;

	GracBlockDevice	*next;
	GracBlockDevice	*prev;
};

static GracBlockDevice	*grac_block_device_list = NULL;

GracBlockDevice*	grac_block_device_alloc();
void		grac_block_device_free(GracBlockDevice** pblk);

static gboolean	grac_block_device_list_add(GracBlockDevice* parent, GracBlockDevice	*blk);
static void		  grac_block_device_list_remove_all();
static GracBlockDevice*	grac_block_device_list_find_parent(char *child_name);

GracBlockDevice*	grac_block_device_alloc()
{
	GracBlockDevice *blk = malloc(sizeof(GracBlockDevice));
	if (blk) {
		c_memset(blk, 0, sizeof(GracBlockDevice));
	}

	return blk;
}

void	grac_block_device_free(GracBlockDevice** pblk)
{
	if (pblk != NULL && *pblk != NULL) {
		GracBlockDevice *blk = *pblk;

		c_free(&blk->dev_name);
		c_free(&blk->dev_type);
		c_free(&blk->mount_point);
//	c_free(&blk->sub_system);

		free(blk);

		*pblk = NULL;
	}
}

gboolean grac_block_device_set_name(GracBlockDevice* blk, char *name)
{
	gboolean done = FALSE;
	if (blk) {
		char *ptr = c_strdup(name, 1024);
		if (ptr) {
			c_free(&blk->dev_name);
			blk->dev_name = ptr;
			done = TRUE;
		}
	}
	return done;
}

gboolean grac_block_device_set_type(GracBlockDevice* blk, char *type)
{
	gboolean done = FALSE;
	if (blk) {
		char *ptr = c_strdup(type, 1024);
		if (ptr) {
			c_free(&blk->dev_type);
			blk->dev_type = ptr;
			done = TRUE;
		}
	}
	return done;
}

gboolean grac_block_device_set_mount_point(GracBlockDevice* blk, char *mount)
{
	gboolean done = FALSE;

	if (blk) {
		char *ptr = c_strdup(mount, 1024);
		if (ptr) {
			c_free(&blk->mount_point);
			blk->mount_point = ptr;

			if (c_strmatch(mount, "/") ||
					c_strcmp(mount, "/boot", 5, -1) == 0)
			{
				blk->this_root = TRUE;
				if (blk->parent)
					blk->parent->child_root = TRUE;
			}

			done = TRUE;
		}
	}
	return done;
}

/*
gboolean grac_block_device_set_sub_system(GracBlockDevice* blk, char *sub)
{
	gboolean done = FALSE;
	if (blk) {
		char *ptr = c_strdup(sub, 1024);
		if (ptr) {
			c_free(&blk->sub_system);
			blk->sub_system = ptr;
			done = TRUE;
		}
	}
	return done;
}
*/

gboolean grac_block_device_set_readonly(GracBlockDevice* blk, gboolean set)
{
	gboolean done = FALSE;
	if (blk) {
		blk->readonly = set;
		done = TRUE;
	}
	return done;
}

gboolean grac_block_device_set_removable(GracBlockDevice* blk, gboolean set)
{
	gboolean done = FALSE;
	if (blk) {
		blk->removable = set;
		done = TRUE;
	}
	return done;
}

/*
gboolean grac_block_device_set_hotplug(GracBlockDevice* blk, gboolean set)
{
	gboolean done = FALSE;
	if (blk) {
		blk->hotplug = set;
		done = TRUE;
	}
	return done;
}
*/

static gboolean	grac_block_device_list_add(GracBlockDevice* parent, GracBlockDevice	*blk)
{
	if (blk == NULL)
		return FALSE;

	blk->next = NULL;
	blk->prev = NULL;
	blk->parent = NULL;
	blk->children = NULL;

	if (parent == NULL) {
		if (grac_block_device_list == NULL) {
			grac_block_device_list = blk;
		}
		else {
			GracBlockDevice	*last;
			last = grac_block_device_list;
			while (last) {
				if (last->next == NULL)
					break;
				last = last->next;
			}
			last->next = blk;
			blk->prev = last;
		}
	}
	else {
		blk->parent = parent;
		if (parent->children == NULL) {
			parent->children = blk;
		}
		else {
			GracBlockDevice	*last;
			last = parent->children;
			while (last) {
				if (last->next == NULL)
					break;
				last = last->next;
			}
			last->next = blk;
			blk->prev = last;
		}
	}

	return TRUE;
}

static void _grac_block_device_list_free(GracBlockDevice	*blk)
{
	GracBlockDevice	*cur, *next;

	cur = blk;
	while (cur) {
		if (cur->children)
			_grac_block_device_list_free(cur->children);
		next = cur->next;
		grac_block_device_free(&cur);
		cur = next;
	}
}

static void	grac_block_device_list_remove_all()
{
	_grac_block_device_list_free(grac_block_device_list);

	grac_block_device_list = NULL;
}


static int grac_block_device_list_count()
{
	int count = 0;

	GracBlockDevice	*cur;

	cur = grac_block_device_list;
	while (cur) {
		count++;
		cur = cur->next;
	}

	return count;
}

static GracBlockDevice*	_grac_block_device_list_find_sub(GracBlockDevice* blk, char *name, gboolean sub)
{
	GracBlockDevice	*find = NULL;
	GracBlockDevice	*cur;

	if (c_strlen(name, 256) > 0) {
		cur = blk;
		while (cur) {
			if (c_strmatch(cur->dev_name, name)) {
				find = cur;
				break;
			}
			if (sub && cur->children) {
				find = _grac_block_device_list_find_sub(cur->children, name, sub);
				if (find)
					break;
			}
			cur = cur->next;
		}
	}

	return find;
}

static GracBlockDevice*	grac_block_device_list_find(char *name, gboolean sub)
{
	return _grac_block_device_list_find_sub(grac_block_device_list, name, sub);
}

static GracBlockDevice*	grac_block_device_list_find_parent(char *child_name)
{
	GracBlockDevice	*find = NULL;
	GracBlockDevice	*cur;

	if (c_strlen(child_name, 256) > 0) {
		cur = grac_block_device_list;
		while (cur) {
			int n = c_strlen(cur->dev_name, 256);
			if (c_strcmp(cur->dev_name, child_name, n, -1) == 0) {
				char *ptr = child_name + n;
				if (*ptr) {
					char	ch = *ptr;
					while (ch) {
						if (ch < '0' || ch > '9')
							break;
						ptr++;
						ch = *ptr;
					}
					if (ch == 0) {
						find = cur;
						break;
					}
				}
			}
			cur = cur->next;
		}
	}

	return find;
}

static gboolean grac_block_device_list_check_root(char *dev_name)
{
	gboolean res = FALSE;
	GracBlockDevice	*find;

	if (c_strlen(dev_name, 256) > 0) {
		find = grac_block_device_list_find(dev_name, TRUE);
		if (find) {
			if (find->this_root || find->child_root)
				res = TRUE;
		}
	}

	return res;
}



static void	_grac_block_device_list_dump_sub(int depth, GracBlockDevice	*blk)
{
	char	tab[256];

	if (blk == NULL)
		return;

	if (depth >= 255)
		return;

	c_memset(tab, '\t', depth);
	tab[depth] = 0;

//char *form = "NAME=\"%s\" TYPE=\"%s\" RM=%d RO=%d MOUNTPOINT=\"%s\" HOTPLUG=%d SUBSYSTEMS=\"%s\"";
	char *form = "NAME=\"%s\" TYPE=\"%s\" RM=%d RO=%d MOUNTPOINT=\"%s\"";

	GracBlockDevice	*cur = blk;
	while (cur) {
		g_printf("%s", tab);
//	g_printf(form, 	cur->dev_name, cur->dev_type, (int)cur->removable, (int)cur->readonly,
//							  cur->mount_point, (int)cur->hotplug, cur->sub_system);
		g_printf(form, 	cur->dev_name, cur->dev_type, (int)cur->removable, (int)cur->readonly, cur->mount_point);
		g_printf("(mounted flag %d:%d)\n", (int)cur->this_root, (int)cur->child_root);

		_grac_block_device_list_dump_sub(depth+1, cur->children);

		cur = cur->next;
	}

}

void	grac_block_device_list_dump()
{
	_grac_block_device_list_dump_sub(0, grac_block_device_list);
}

static gboolean _make_block_device_list_parent()
{
	char	output[4096];
	gboolean done;
	char	*cmd;

	cmd = "/bin/lsblk -d -n -o name";
	done =  sys_run_cmd_get_output(cmd, "test", output, sizeof(output));
	if (done == FALSE) {
		grac_log_error("commamd running error : %s", cmd);
		return FALSE;
	}

	char	*ptr = output;
	while (*ptr) {
		int	n;
		char *t = c_strchr(ptr, '\n', 256);
		if (t)
			*t = 0;
		n = c_strlen(ptr, 256);
		if (n > 0) {
			GracBlockDevice*	dev = grac_block_device_alloc();
			if (dev == NULL) {
				grac_log_error("can't alloc data for block device");
				done = FALSE;
				break;
			}
			done = grac_block_device_set_name(dev, ptr);
			if (done == FALSE) {
				grac_log_error("can't set device name");
				break;
			}
			done = grac_block_device_list_add(NULL, dev);
			if (done == FALSE) {
				grac_log_error("can't add a device info to list");
				break;
			}
		}
		ptr += n+1;
	}

	if (done) {
		int cnt = grac_block_device_list_count();
		if (cnt == 0) {
			grac_log_error("no parent data");
			done = FALSE;
		}
	}

	return done;
}

static	int	_get_key_value(char *buf, char *key, char *val, int size)
{
	gboolean res = FALSE;
	int	buf_idx;
	int	key_idx = 0, val_idx = 0;;
	int	ch;

	*key = 0;
	*val = 0;

	// skip blank
	buf_idx = 0;
	while(1) {
		ch = buf[buf_idx] & 0x0ff;
		if (ch == 0)
			break;
		if (ch > 0x20)
			break;
		buf_idx++;
	}

	// collect key
	key_idx = 0;
	while(key_idx < size-1) {
		ch = buf[buf_idx] & 0x0ff;
		if (ch >= 0 && ch <= 0x20)
			break;
		buf_idx++;
		if (ch == '=')
			break;
		key[key_idx] = ch;
		key_idx++;
	}
	key[key_idx] = 0;

	if (ch == '=' && buf[buf_idx] == '\"') {
		buf_idx++;
		// collect value
		val_idx = 0;
		while(val_idx < size-1) {
			ch = buf[buf_idx] & 0x0ff;
			if (ch >= 0 && ch <= 0x20)
				break;
			buf_idx++;
			if (ch == '\"')
				break;
			val[val_idx] = ch;
			val_idx++;
		}
		val[val_idx] = 0;
		if (ch == '\"')
			res = TRUE;
		else
			res = FALSE;
	}
	else {
		res = FALSE;
	}

	if (key_idx == 0 || res == FALSE)
		return -1;
	else
		return buf_idx;
}

static gboolean _make_block_device_list_complete_one(char *buf)
{
	gboolean done = TRUE;
	gboolean res;
	GracBlockDevice* blk;

	if (buf == NULL)
		return FALSE;

	int		idx;
	int		off = 0, used_cnt;
	char	key[256], val[256];
	int		fld_cnt = 5;	// 7 for hotplug, subsystem

	for (idx=0; idx<fld_cnt; idx++) {
		used_cnt = _get_key_value(buf+off, key, val, sizeof(key));
		if (used_cnt <= 0) {
			grac_log_error("Invalid line : %s", buf);
			break;
		}

		if (idx == 0) {
			if (c_strimatch(key, "NAME") == 0) {
				grac_log_error("Invalid line : %s", buf);
				break;
			}
		}

		if (c_strimatch(key, "NAME")) {
			blk = grac_block_device_list_find(val, FALSE);
			if (blk == NULL) {
					GracBlockDevice* parent;
					parent = grac_block_device_list_find_parent(val);
					if (parent == NULL) {
						grac_log_error("unknown parent device for %s", buf);
						break;
					}
					blk = grac_block_device_alloc();
					if (blk == NULL) {
						grac_log_error("Out of memory : %s", val);
						break;
					}
					res = grac_block_device_list_add(parent, blk);
					if (res == FALSE) {
						grac_log_error("Can't add child : %s", val);
						break;
					}
					res = grac_block_device_set_name(blk, val);
					if (res == FALSE) {
						grac_log_error("Can't set name : %s", val);
						break;
					}
			}
		}
		else if (c_strimatch(key, "TYPE")) {
			res = grac_block_device_set_type(blk, val);
			if (res == FALSE) {
				grac_log_error("Can't set type : %s", buf);
				break;
			}
		}
		else if (c_strimatch(key, "RM")) {
			if (c_strmatch(val, "1"))
				res = grac_block_device_set_removable(blk, TRUE);
			else
				res = grac_block_device_set_removable(blk, FALSE);
			if (res == FALSE) {
				grac_log_error("Can't set removable : %s", buf);
				break;
			}
		}
		else if (c_strimatch(key, "RO")) {
			if (c_strmatch(val, "1"))
				res = grac_block_device_set_readonly(blk, TRUE);
			else
				res = grac_block_device_set_readonly(blk, FALSE);
			if (res == FALSE) {
				grac_log_error("Can't set readonly : %s", buf);
				break;
			}
		}
		else if (c_strimatch(key, "MOUNTPOINT")) {
			res = grac_block_device_set_mount_point(blk, val);
			if (res == FALSE) {
				grac_log_error("Can't set mount_point : %s", buf);
				break;
			}
		}
	/*
		else if (c_strimatch(key, "HOTPLUG")) {
			if (c_strmatch(val, "1"))
				res = grac_block_device_set_hotplug(blk, TRUE);
			else
				res = grac_block_device_set_hotplug(blk, FALSE);
			if (res == FALSE) {
				grac_log_error("Can't set hotplug : %s", buf);
				break;
			}
		}
		else if (c_strimatch(key, "SUBSYSTEMS")) {
			res = grac_block_device_set_sub_system(blk, val);
			if (res == FALSE) {
				grac_log_error("Can't set sub_system : %s", buf);
				break;
			}
		}
	*/
		else {
			grac_log_error("Invalid line  : %s", buf);
			break;
		}

		off += used_cnt;
	}

	if (idx != fld_cnt) {
		done = FALSE;
	}

	return done;
}

static gboolean _make_block_device_list_complete()
{
	char	output[4096];
	gboolean done = TRUE;
	char	*cmd;

//cmd = "/bin/lsblk -P -n -o name,type,rm,ro,mountpoint,hotplug,subsystems";
	cmd = "/bin/lsblk -P -n -o name,type,rm,ro,mountpoint";
	done =  sys_run_cmd_get_output(cmd, "test", output, sizeof(output));
	if (done == FALSE) {
		grac_log_error("commamd running error : %s", cmd);
		return FALSE;
	}

	// output of command
	// NAME="sr0" TYPE="rom" RM="1" RO="0" MOUNTPOINT="" HOTPLUG="1" SUBSYSTEMS="block:scsi:pci"

	char	*ptr = output;
	while (*ptr) {
		int	n;
		char *t = c_strchr(ptr, '\n', 256);
		if (t)
			*t = 0;
		n = c_strlen(ptr, 256);
		if (n > 0) {
			done = _make_block_device_list_complete_one(ptr);
			if (done == FALSE) {
				grac_log_error("Invalid line : %s", ptr);
				return FALSE;
			}
		}
		ptr += n+1;
	}

	return done;
}

gboolean grac_block_device_list_make()
{
	gboolean done;

	// 1st step only get main device without children
	done = _make_block_device_list_parent();
	if (done == FALSE) {
		grac_block_device_list_remove_all();
		return FALSE;
	}

	// 2nd step  : fill parent's information, and add children
	done = _make_block_device_list_complete();
	if (done == FALSE) {
		grac_block_device_list_remove_all();
		return FALSE;
	}

	return done;
}

static gboolean	do_adjust_udev_rule_file(char *org_path, char *adj_path)
{
	gboolean done = TRUE;
	FILE	*in = NULL;
	FILE	*out = NULL;
	char	buf[2048];
	char	*ptr, *ptr2;
	char	match[256], chk_dev[256];
	int		idx, ch;
	char	*key = "KERNEL==";
	char	*subsystem = "SUBSYSTEM==\"block\"";

	in = g_fopen(org_path, "r");
	if (in == NULL) {
		grac_log_error("%s() : can't open file : %s", __FUNCTION__, org_path);
		return FALSE;
	}

	out = g_fopen(adj_path, "w");
	if (out == NULL) {
		grac_log_error("%s() : can't create file : %s", __FUNCTION__, adj_path);
		fclose(in);
		return FALSE;
	}

	while (1) {
		if (fgets(buf, sizeof(buf), in) == NULL)
			break;

		ptr = c_strstr(buf, subsystem, sizeof(buf));
		if (ptr == NULL) {
			fprintf(out, "%s", buf);
			continue;
		}

		ptr = c_strstr(buf, key, sizeof(buf));
		if (ptr == NULL) {
			fprintf(out, "%s", buf);
			continue;
		}

		ptr += c_strlen(key, sizeof(buf));
		if (*ptr != '\"') {
			fprintf(out, "%s", buf);
			continue;
		}
		ptr++;	// skip ""

		ptr2 = ptr;
		idx = 0;
		while (1) {
			ch = *ptr2;
			if (ch == '\"' || ch == 0)
				break;
			match[idx++] = ch;
			ptr2++;
		}
		match[idx] = 0;

		int off;
		for (off=0; off<idx; off++) {
			ch = match[off];
			if (ch == '[')
				break;
			chk_dev[off] = ch;
		}
		chk_dev[off] = 0;

		if (ch != '[') {
			if (grac_block_device_list_check_root(chk_dev)) {
				if (buf[0] != '#')
					fprintf(out, "### ");
			}
			fprintf(out, "%s", buf);
			continue;
		}

		int	dev_ch;
		for (dev_ch=0; dev_ch<26; dev_ch++) {
			chk_dev[off] = 'a' + dev_ch;
			chk_dev[off+1] = 0;
			if (grac_block_device_list_check_root(chk_dev) == FALSE)
				break;
		}
		if (dev_ch == 26) {
			if (buf[0] != '#')
				fprintf(out, "### ");
			fprintf(out, "%s", buf);
		}
		else if (match[off+1] < 'a' || match[off+1] > 'z') {
			fprintf(out, "%s", buf);
		}
		else {
			*ptr = 0;
			match[off+1] = 'a' + dev_ch;
			fprintf(out, "%s%s%s", buf, match, ptr2);
		}
	}

	if (in)
		fclose(in);
	if (out)
		fclose(out);

	return done;
}


gboolean	grac_adjust_udev_rule_file(char *org_path, char *adj_path)
{
	gboolean res;

	grac_block_device_list_make();
	//grac_block_device_list_dump();

	res = do_adjust_udev_rule_file(org_path, adj_path);

	grac_block_device_list_remove_all();

	return res;
}
