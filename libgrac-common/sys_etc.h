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
 * sys_etc.h
 *
 *  Created on: 2016. 10. 12
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_COMMON_SYS_ETC_H_
#define LIBGRAC_COMMON_SYS_ETC_H_

#include <stdio.h>
#include <glib.h>

gboolean sys_get_current_process_name(gchar *name, int size);

gboolean sys_run_cmd_no_output(gchar *cmd, char *logstr);
gboolean sys_run_cmd_get_output(gchar *cmd, char *logstr, char *output, int size);

FILE* 	 sys_popen(char *cmd, char *type, int *pid);
gboolean sys_pclose(FILE *fp, int pid);

int	sys_getpid();
int	sys_gettid();


#endif // LIBGRAC_COMMON_SYS_ETC_H_
