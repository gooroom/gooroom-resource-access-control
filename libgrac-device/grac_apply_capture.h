/*
 * grac_apply_capture_.h
 *
 *  Created on: 2016. 1. 15.
 *   Modified on 2016. 9. 5
 *      Author: user
 */

#ifndef _grac_apply_capture_H_
#define _grac_apply_capture_H_

#include <glib.h>

gboolean grac_apply_capture_init(gchar *captureAppPath);
void     grac_apply_capture_end();
gboolean grac_apply_capture_allow_by_uid(uid_t uid, gboolean allow);
gboolean grac_apply_capture_allow_by_gid(uid_t gid, gboolean allow);
gboolean grac_apply_capture_allow_by_user (char *user,  gboolean allow);
gboolean grac_apply_capture_allow_by_group(char *group, gboolean allow);


#endif /* _grac_apply_capture_H_ */
