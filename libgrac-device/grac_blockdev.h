/*
 * grac_blockdev.h
 *
 *  Created on: 2017. 10. 30.
 *      Author: yang
 */

#ifndef _GRAC_BLOCKDEV_H_
#define _GRAC_BLOCKDEV_H_

#include <glib.h>

gboolean grac_blockdev_init();
gboolean grac_blockdev_apply_disallow();
gboolean grac_blockdev_apply_readonly();
gboolean grac_blockdev_apply_normal();
gboolean grac_blockdev_end();

gboolean grac_udev_check_existing(char *sysname);
gboolean grac_udev_get_type(char *sysname, char *type, int type_size);
gboolean grac_udev_get_node(char *sysname, char *node, int node_size);
int	     grac_udev_get_removable(char *sysname);		// return -1, 0, 1
gboolean grac_udev_get_parent_sysname(char *sysname, char *parent, int parent_size);
int	     grac_udev_get_children_sysnames(char *sysname, char *children, int children_size);   // delmeter ";"
gboolean grac_udev_get_mount_point(char *sysname, char *mount, int mount_size);


#endif /* _GRAC_BLOCKDEV_H_ */
