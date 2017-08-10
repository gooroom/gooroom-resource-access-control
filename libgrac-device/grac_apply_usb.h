/*
 * grac_apply_usb.h
 *
 *  Created on: 2016. 1. 15.
 *   Modified on 2016. 9. 5
 *      Author: user
 */

#ifndef _grac_apply_usb_H_
#define _grac_apply_usb_H_

#include <glib.h>

gboolean grac_apply_usb_init();
void     grac_apply_usb_end();
gboolean grac_apply_usb_storage_allow_by_uid  (uid_t  usb_uid, uid_t  allow_uid, gboolean allow);
gboolean grac_apply_usb_storage_allow_by_user (gchar* usb_usr, gchar* allow_usr, gboolean allow);
gboolean grac_apply_usb_storage_allow_by_gid  (uid_t  usb_uid, gid_t  allow_gid, gboolean allow);
gboolean grac_apply_usb_storage_allow_by_group(gchar* usb_usr, gchar* allow_grp, gboolean allow);

#endif /* _grac_apply_usb_H_ */
