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
 * grac_blockdev.h
 *
 *  Created on: 2017. 10. 30.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_DEVICE_GRAC_BLOCKDEV_H_
#define LIBGRAC_DEVICE_GRAC_BLOCKDEV_H_

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


#endif // LIBGRAC_DEVICE_GRAC_BLOCKDEV_H_
