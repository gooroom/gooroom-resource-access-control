/*
 * sys_etc.h
 *
 *  Created on: 2016. 10. 12
 *      Author: user
 */

#ifndef _SYS_ETC_H_
#define _SYS_ETC_H_

#include <glib.h>

gboolean sys_get_current_process_name(gchar *name, int size);


gboolean sys_run_cmd_no_output (gchar *cmd, char *logstr);
gboolean sys_run_cmd_get_output(gchar *cmd, char *logstr, char *output, int size);

int sys_check_running_process(char *username, char *exec);		// -1, 0, 1

#endif /* _SYS_ETC_H_ */
