/*
 * sys_etc.h
 *
 *  Created on: 2016. 10. 12
 *      Author: user
 */

#ifndef _SYS_ETC_H_
#define _SYS_ETC_H_

#include <stdio.h>
#include <glib.h>

gboolean sys_get_current_process_name(gchar *name, int size);

gboolean sys_run_cmd_no_output (gchar *cmd, char *logstr);
gboolean sys_run_cmd_get_output(gchar *cmd, char *logstr, char *output, int size);

gboolean sys_run_cmd(gchar *cmd, long wait, char *logstr, char *output, int size);

int sys_check_running_process(char *username, char *exec);		// -1, 0, 1

FILE* 	 sys_popen (char *cmd, char *type, int *pid);
gboolean sys_pclose(FILE *fp, int pid);


#endif /* _SYS_ETC_H_ */
