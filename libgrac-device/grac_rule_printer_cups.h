/*
 * grac_rule_printer_cups.h
 *
 *  Created on: 2017. 10. 23.
 *      Author: yang
 */

#ifndef _GRAC_RULE_PRINTER_CUPS_H_
#define _GRAC_RULE_PRINTER_CUPS_H_

#include <glib.h>

gboolean grac_rule_printer_cups_init ();
void     grac_rule_printer_cups_final();
gboolean grac_rule_printer_cups_apply(gboolean allow);

gboolean grac_rule_printer_cups_add_uid  (uid_t uid, gboolean allow);
gboolean grac_rule_printer_cups_add_gid  (uid_t gid, gboolean allow);
gboolean grac_rule_printer_cups_add_user (gchar *user,  gboolean allow);
gboolean grac_rule_printer_cups_add_group(gchar *group, gboolean allow);
gboolean grac_rule_printer_cups_del_uid  (uid_t uid);
gboolean grac_rule_printer_cups_del_gid  (uid_t gid);
gboolean grac_rule_printer_cups_del_user (gchar *user);
gboolean grac_rule_printer_cups_del_group(gchar *group);

#endif /* _GRAC_RULE_PRINTER_CUPS_H_ */
