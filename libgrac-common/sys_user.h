/*
 * sys_user.h
 *
 *  Created on: 2015. 11. 19.
 *      Author: user
 */

#ifndef _SYS_USER_H_
#define _SYS_USER_H_

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


#endif /* _SYS_USER_H_ */
