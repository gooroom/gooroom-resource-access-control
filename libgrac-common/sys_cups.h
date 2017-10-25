/*
 * sys_cups.h
 *
 *  Created on: 2016. 1. 18.
 *      Author: user
 */

/*
 *  Todo
 *  		printer name
 */

#ifndef _SYS_CUPS_H_
#define _SYS_CUPS_H_

#include <glib.h>

gboolean sys_cups_access_init();
void		 sys_cups_access_final();

gboolean sys_cups_access_add_user (gchar *user,  gboolean allow);
gboolean sys_cups_access_add_group(gchar *group, gboolean allow);

gboolean sys_cups_access_apply(gboolean allow);		// all printers
gboolean sys_cups_access_apply_by_index(int idx, gboolean allow);
gboolean sys_cups_access_apply_by_name(char *name, gboolean allow);

int			 sys_cups_printer_count();
const char* sys_cups_get_printer_name(int idx);


#endif /* _SYS_CUPS_H_ */
