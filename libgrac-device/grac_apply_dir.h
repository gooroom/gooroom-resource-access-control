/*
 * grac_apply_dir.h
 *
 *  Created on: 2016. 1. 15.
 *   Modified on 2016. 9. 5
 *      Author: user
 */

#ifndef _grac_apply_dir_H_
#define _grac_apply_dir_H_

#include <glib.h>

gboolean grac_apply_dir_init();
void     grac_apply_dir_end();
gboolean grac_apply_dir_home_init();	// 2016.4.
gboolean grac_apply_dir_home_allow_by_uid  (uid_t  home_uid, uid_t  allow_uid, gboolean allow);
gboolean grac_apply_dir_home_allow_by_user (gchar* home_usr, gchar* allow_usr, gboolean allow);
gboolean grac_apply_dir_home_allow_by_gid  (gid_t  gid,   gboolean allow);
gboolean grac_apply_dir_home_allow_by_group(gchar* group, gboolean allow);
//gboolean grac_apply_dir_home_allow_by_gid  (uid_t  home_uid, gid_t  allow_gid, gboolean allow);
//gboolean grac_apply_dir_home_allow_by_group(gchar* home_usr, gchar* allow_grp, gboolean allow);


#endif /*  _grac_apply_dir_H_ */
