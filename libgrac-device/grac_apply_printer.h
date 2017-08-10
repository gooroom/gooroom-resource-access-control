/*
 * grac_apply_printer.h
 *
 *  Created on: 2016. 1. 15.
 *   Modified on 2016. 9. 5
 *      Author: user
 */

#ifndef _grac_apply_printer_H_
#define _grac_apply_printer_H_

#include <glib.h>

gboolean grac_apply_printer_access_init();
void     grac_apply_printer_access_end();
gboolean grac_apply_printer_access_apply (gboolean allow);
gboolean grac_apply_printer_access_add_uid  (uid_t uid, gboolean allow);
gboolean grac_apply_printer_access_add_gid  (uid_t gid, gboolean allow);
gboolean grac_apply_printer_access_add_user (gchar *user,  gboolean allow);
gboolean grac_apply_printer_access_add_group(gchar *group, gboolean allow);
gboolean grac_apply_printer_access_del_uid  (uid_t uid);
gboolean grac_apply_printer_access_del_gid  (uid_t gid);
gboolean grac_apply_printer_access_del_user (gchar *user);
gboolean grac_apply_printer_access_del_group(gchar *group);


#endif /* _grac_apply_printer_H_ */
