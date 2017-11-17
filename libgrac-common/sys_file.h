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
 * sys_file.h
 *
 *  Created on: 2015. 11. 19.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_COMMON_SYS_FILE_H_
#define LIBGRAC_COMMON_SYS_FILE_H_

#include <glib.h>

gboolean sys_file_get_default_home_dir(gchar *user, gchar *homeDir, int size);
gboolean sys_file_change_owner_group(gchar *path, gchar* owner, gchar* group);
gboolean sys_file_get_owner_group(gchar *path, gchar *owner, int oSize,
		                                           gchar *group, int gSize);

gboolean sys_file_get_default_usb_dir(gchar *user, gchar *homeDir, int size);

gboolean sys_file_set_mode(gchar *path, gchar *mode);
gboolean sys_file_set_mode_user(gchar *path, gchar *mode);	// process only owner
gboolean sys_file_set_mode_group(gchar *path, gchar *mode); // process only group
gboolean sys_file_set_mode_other(gchar *path, gchar *mode); // process only other

gboolean sys_file_make_new_directory(gchar *path, gchar* owner, gchar* group, int mode);  // 존재하면 아무 동작 없음

int			 sys_file_get_length(const gchar *path);
gboolean sys_file_is_existing(const gchar *path);

int		sys_file_open(const char *path, char *mode);
int		sys_file_read(int fd, char *buf, int size);
void	sys_file_close(int *fd);

#endif // LIBGRAC_COMMON_SYS_FILE_H_
