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
 * sys_cups.h
 *
 *  Created on: 2016. 1. 18.
 *      Author: gooroom@gooroom.kr
 */

/*
 *  Todo
 *  		printer name
 */

#ifndef LIBGRAC_COMMON_SYS_CUPS_H_
#define LIBGRAC_COMMON_SYS_CUPS_H_

#include <glib.h>

gboolean sys_cups_access_init();
void		 sys_cups_access_final();

gboolean sys_cups_access_add_user(gchar *user,  gboolean allow);
gboolean sys_cups_access_add_group(gchar *group, gboolean allow);

gboolean sys_cups_access_apply(gboolean allow);		// all printers
gboolean sys_cups_access_apply_by_index(int idx, gboolean allow);
gboolean sys_cups_access_apply_by_name(char *name, gboolean allow);

int			 sys_cups_printer_count();
const char* sys_cups_get_printer_name(int idx);


#endif // LIBGRAC_COMMON_SYS_CUPS_H_
