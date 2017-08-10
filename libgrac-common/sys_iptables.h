/*
 * sys_iptblables.h
 *
 *  Created on: 2016. 1. 17.
 *      Author: user
 */

#ifndef _SYS_IPTABLES_H_
#define _SYS_IPTABLES_H_

#include <glib.h>

//  bit combination
#define IPTBL_FILTER_INPUT    0x01
#define IPTBL_FILTER_FORWARD  0x02
#define IPTBL_FILTER_OUTPUT   0x04
#define IPTBL_FILTER_ALL      (IPTBL_FILTER_INPUT|IPTBL_FILTER_FORWARD|IPTBL_FILTER_OUTPUT)

#define IPTBL_ACTION_ACCEPT   1
#define IPTBL_ACTION_DROP     2

gboolean sys_iptbl_init();
void     sys_iptbl_clear();

gboolean sys_iptbl_set_action_by_user (gchar *name, int filter, int action);
gboolean sys_iptbl_set_action_by_uid  (uid_t uid,   int filter, int action);

// not yet
//gboolean sys_iptbl_get_action_by_user (gchar *name, int *filter, int *action);
//gboolean sys_iptbl_get_action_by_uid  (uid_t uid,   int *filter, int *action);

gboolean sys_iptbl_del_all_action_by_user(gchar *name);
gboolean sys_iptbl_del_all_action_by_uid (uid_t uid);

gboolean sys_iptbl_set_action_by_group(gchar *name, int filter, int action);
gboolean sys_iptbl_set_action_by_gid  (gid_t gid,   int filter, int action);

// not yet
//gboolean sys_iptbl_get_action_by_group(gchar *name, int *filter, int *action);
//gboolean sys_iptbl_get_action_by_gid  (gid_t gid,   int *filter, int *action);

gboolean sys_iptbl_del_all_action_by_group(gchar *name);
gboolean sys_iptbl_del_all_action_by_gid  (gid_t gid);

#endif /* _SYS_IPTABLES_H_ */
