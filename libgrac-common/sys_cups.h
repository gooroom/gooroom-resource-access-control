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

#define CUPS_ACCESS_ALLOW  1
#define CUPS_ACCESS_DENY   2

gboolean sys_cups_access_init();
void		 sys_cups_access_end();

gboolean sys_cups_access_add_user (gchar *user,  int access);
gboolean sys_cups_access_add_group(gchar *group, int access);

gboolean sys_cups_access_apply(int access);		// all printers
gboolean sys_cups_access_apply_by_index(int idx, int access);
gboolean sys_cups_access_apply_by_name(char *name, int access);

int			 sys_cups_printer_count();
const char* sys_cups_get_printer_name(int idx);


#endif /* _SYS_CUPS_H_ */
