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
 * sys_user.h
 *
 *  Created on: 2015. 11. 19.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_COMMON_SYS_USER_H_
#define LIBGRAC_COMMON_SYS_USER_H_

#include <unistd.h>
#include <sys/types.h>
#include <glib.h>

// notice! not use uid_t and gid_t

gboolean sys_user_get_name_from_uid(int uid, gchar* name, int maxlen);
gboolean sys_user_get_name_from_gid(int gid, gchar* name, int maxlen);

// return -1, if error
int   sys_user_get_uid_from_name(gchar* name);
int   sys_user_get_gid_from_name(gchar* name);

gboolean sys_user_get_login_name(char *name, int size);
int			 sys_user_get_login_uid();


#endif // LIBGRAC_COMMON_SYS_USER_H_
