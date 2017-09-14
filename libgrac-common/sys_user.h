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

gboolean sys_user_get_user_group_list(gchar* user, gchar *groups, int maxlen);
int      sys_user_get_user_group_list_len(gchar* user);       // return 0 or n

gboolean sys_user_add_user_group(gchar* name, gchar* group);
gboolean sys_user_del_user_group(gchar* name, gchar* group);
//gboolean sys_user_is_user_group (gchar* name, gchar* group);

gboolean sys_user_is_in_group(gchar* usrname, gchar* grpname);

void sys_user_get_uid_range(int* min, int *max);
void sys_user_get_gid_range(int* min, int *max);
void sys_user_get_sys_uid_range(int* min, int *max);  //except root and only ids below uid_min
void sys_user_get_sys_gid_range(int* min, int *max);  //except root and only ids below gid_min


int	sys_user_get_main_uid (gchar *group);
gboolean sys_user_get_main_group_from_uid(int uid, gchar *group, int maxlen);
gboolean sys_user_get_main_group_from_name(gchar *name, gchar *group, int maxlen);


gboolean sys_user_check_home_dir_by_uid(int uid);
gboolean sys_user_check_home_dir_by_name(char *name);

gboolean sys_user_get_home_dir_by_uid(int uid, char *dir, int size);
gboolean sys_user_get_home_dir_by_name(char *name, char *dir, int size);

//gboolean sys_check_login(char *user, char *pwd);

gboolean sys_user_get_login_name_by_api(char *name, int size);
gboolean sys_user_get_login_name_by_wcmd(char *name, int size);

gboolean sys_user_get_login_name(char *name, int size);
int			 sys_user_get_login_uid();


#endif /* _SYS_USER_H_ */
