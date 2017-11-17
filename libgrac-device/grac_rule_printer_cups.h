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
 * grac_rule_printer_cups.h
 *
 *  Created on: 2017. 10. 23.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_DEVICE_GRAC_RULE_PRINTER_CUPS_H_
#define LIBGRAC_DEVICE_GRAC_RULE_PRINTER_CUPS_H_

#include <glib.h>

gboolean grac_rule_printer_cups_init();
void     grac_rule_printer_cups_final();
gboolean grac_rule_printer_cups_apply(gboolean allow);

gboolean grac_rule_printer_cups_add_uid(uid_t uid, gboolean allow);
gboolean grac_rule_printer_cups_add_gid(uid_t gid, gboolean allow);
gboolean grac_rule_printer_cups_add_user(gchar *user,  gboolean allow);
gboolean grac_rule_printer_cups_add_group(gchar *group, gboolean allow);
gboolean grac_rule_printer_cups_del_uid(uid_t uid);
gboolean grac_rule_printer_cups_del_gid(uid_t gid);
gboolean grac_rule_printer_cups_del_user(gchar *user);
gboolean grac_rule_printer_cups_del_group(gchar *group);

#endif // LIBGRAC_DEVICE_GRAC_RULE_PRINTER_CUPS_H_
