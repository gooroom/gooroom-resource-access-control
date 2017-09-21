/*
 * test_udevmap_cnv.c
 *
 *  Created on: 2017. 9. 20.
 *      Author: yang
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#include "grac_config.h"
#include "cutility.h"
#include "sys_etc.h"

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
static void			grac_block_device_list_dump();
static GracBlockDevice*	grac_block_device_list_find_parent(char *child_name);

GracBlockDevice*	grac_block_device_alloc()
{
	GracBlockDevice *blk = malloc(sizeof(GracBlockDevice));
	if (blk) {
		memset(blk, 0, sizeof(GracBlockDevice));
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
		printf("%s", tab);
//	printf(form, 	cur->dev_name, cur->dev_type, (int)cur->removable, (int)cur->readonly,
//							  cur->mount_point, (int)cur->hotplug, cur->sub_system);
		printf(form, 	cur->dev_name, cur->dev_type, (int)cur->removable, (int)cur->readonly, cur->mount_point);
		printf("(mounted flag %d:%d)\n", (int)cur->this_root, (int)cur->child_root);

		_grac_block_device_list_dump_sub(depth+1, cur->children);

		cur = cur->next;
	}

}

static void	grac_block_device_list_dump()
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
		printf("commamd running error : %s\n", cmd);
		return FALSE;
	}

	char	*ptr = output;
	while (*ptr) {
		int	n;
		char *t = c_strchr(ptr, '\n', 256);
		if (t)
			*t = 0;
		n = strlen(ptr);
		if (n > 0) {
			GracBlockDevice*	dev = grac_block_device_alloc();
			if (dev == NULL) {
				printf("can't alloc data for block device\n");
				done = FALSE;
				break;
			}
			done = grac_block_device_set_name(dev, ptr);
			if (done == FALSE) {
				printf("can't set device name\n");
				break;
			}
			done = grac_block_device_list_add(NULL, dev);
			if (done == FALSE) {
				printf("can't add a device info to list\n");
				break;
			}
		}
		ptr += n+1;
	}

	if (done) {
		int cnt = grac_block_device_list_count();
		if (cnt == 0) {
			printf("no parent data \n");
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
			printf("Invalid line  : %s\n", buf);
			break;
		}

		if (idx == 0) {
			if (c_strimatch(key, "NAME") == 0) {
				printf("Invalid line  : %s\n", buf);
				break;
			}
		}

		if (c_strimatch(key, "NAME")) {
			blk = grac_block_device_list_find(val, FALSE);
			if (blk == NULL) {
					GracBlockDevice* parent;
					parent = grac_block_device_list_find_parent(val);
					if (parent == NULL) {
						printf("unknown parent device for %s\n", buf);
						break;
					}
					blk = grac_block_device_alloc();
					if (blk == NULL) {
						printf("Out of memory : %s\n", val);
						break;
					}
					res = grac_block_device_list_add(parent, blk);
					if (res == FALSE) {
						printf("Can't add child : %s\n", val);
						break;
					}
					res = grac_block_device_set_name(blk, val);
					if (res == FALSE) {
						printf("Can't set name : %s\n", val);
						break;
					}
			}
		}
		else if (c_strimatch(key, "TYPE")) {
			res = grac_block_device_set_type(blk, val);
			if (res == FALSE) {
				printf("Can't set type : %s\n", buf);
				break;
			}
		}
		else if (c_strimatch(key, "RM")) {
			if (c_strmatch(val, "1"))
				res = grac_block_device_set_removable(blk, TRUE);
			else
				res = grac_block_device_set_removable(blk, FALSE);
			if (res == FALSE) {
				printf("Can't set removable : %s\n", buf);
				break;
			}
		}
		else if (c_strimatch(key, "RO")) {
			if (c_strmatch(val, "1"))
				res = grac_block_device_set_readonly(blk, TRUE);
			else
				res = grac_block_device_set_readonly(blk, FALSE);
			if (res == FALSE) {
				printf("Can't set readonly : %s\n", buf);
				break;
			}
		}
		else if (c_strimatch(key, "MOUNTPOINT")) {
			res = grac_block_device_set_mount_point(blk, val);
			if (res == FALSE) {
				printf("Can't set mount_point : %s\n", buf);
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
				printf("Can't set hotplug : %s\n", buf);
				break;
			}
		}
		else if (c_strimatch(key, "SUBSYSTEMS")) {
			res = grac_block_device_set_sub_system(blk, val);
			if (res == FALSE) {
				printf("Can't set sub_system : %s\n", buf);
				break;
			}
		}
	*/
		else {
			printf("Invalid line  : %s\n", buf);
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
		printf("commamd running error : %s\n", cmd);
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
		n = strlen(ptr);
		if (n > 0) {
			done = _make_block_device_list_complete_one(ptr);
			if (done == FALSE) {
				printf("Invalid line : %s\n", ptr);
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

// ATTRS{address}=="00:15:83:d0:ef:25"
gboolean check_bluetooth_addr_line(char *buf, int bsize, char *format, int fsize)
{
	gboolean res = FALSE;
	char	*subsystem = "SUBSYSTEM==\"bluetooth\"";
	char	*addr_key = "ATTRS{address}==\"";
	char	*ptr1;
	char	*ptr2;
	int		n, ch;

	if (c_strstr(buf, subsystem, bsize) == NULL)
		return FALSE;

	ptr1 = c_strstr(buf, addr_key, bsize);
	if (ptr1 == NULL)
		return FALSE;

	n = c_strlen(addr_key, 256);
	ptr1 += n;
	ptr2 = ptr1;
	while (1) {
		ch = *ptr2;
		if (ch == '\"')
			break;
		if (ch == 0)
			break;
		ptr2++;
	}

	if (ch != '\"')
		return FALSE;

	int	save_ch;
	save_ch = *(ptr1-1);
	*(ptr1-1) = 0;		// end mark on location of '\"'
	ptr2++;				    // move loaction after '\"'
	snprintf(format, fsize, "%s%s%s", buf, "\"%s\"", ptr2);
	*(ptr1-1) = save_ch;

	res = FALSE;
	n = c_strlen(format, fsize);
	if (n > 0 && n < fsize) {
		if (format[n-1] != '\n') {
			if (n < fsize-1) {
				format[n] = '\n';
				format[n+1] = 0;
				res = TRUE;
			}
		}
		else {
			res = TRUE;
		}
	}

	return res;
}

void	adjust_map(char *org, char *adj)
{
	FILE	*in = NULL;
	FILE	*out = NULL;
	char	buf[2048];
	char  mac_format[2048];
	char	*ptr, *ptr2;
	char	*key = "KERNEL==";
	char	match[256], chk_dev[256];
	int		idx, ch;
	char	*subsystem = "SUBSYSTEM==\"block\"";

	in = fopen(org, "r");
	out = fopen(adj, "w");

	while (1) {
		if (fgets(buf, sizeof(buf), in) == NULL)
			break;

		if (check_bluetooth_addr_line(buf, sizeof(buf), mac_format, sizeof(mac_format))) {
			fprintf(out, mac_format, "12:34:56:78:9A:BC");
			fprintf(out, mac_format, "34:56:78:9A:BC:12");
			continue;
		}

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

		ptr += strlen(key);
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

}


void test_udev_map_cnv()
{
	grac_block_device_list_make();

	grac_block_device_list_dump();

	char	*map_org = (char*)grac_config_path_udev_map_org();
	char	*map_local = (char*)grac_config_path_udev_map_local();

	printf("%s %s\n", map_org, map_local);

	adjust_map("test_org.map", "test_adj.map");

	grac_block_device_list_remove_all();
}


