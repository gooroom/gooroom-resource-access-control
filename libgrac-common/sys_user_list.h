/*
 * sys_user_list.h
 *
 *  Created on: 2016. 4. 12.
 *      Author: user
 */

#ifndef _SYS_USER_LIST_H_
#define _SYS_USER_LIST_H_

#include <glib.h>


#define SYS_USER_KIND_GENERAL 0x01
#define SYS_USER_KIND_SYSTEM  0x02
//#define SYS_USER_KIND_SECURE  0x04   // not yet
#define SYS_USER_KIND_ALL     -1

struct sys_user_list* sys_user_list_alloc(int kind);
void   sys_user_list_free(struct sys_user_list** list);
int    sys_user_list_count(struct sys_user_list* list);
char*  sys_user_list_get_name(struct sys_user_list* list, int idx);
int    sys_user_list_get_uid (struct sys_user_list* list, int idx);     // if error, return -1
int    sys_user_list_find_name(struct sys_user_list* list, char *user);	// return index, if error, -1
int    sys_user_list_find_uid (struct sys_user_list* list, int uid);	// return index, if error, -1


#endif /* _SYS_USER_LIST_H_ */
