/*
 * test_etc.c
 *
 *  Created on: 2017. 11. 3.
 *      Author: yang
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "cutility.h"
#include "sys_etc.h"
#include "grac_log.h"

/*
void t_runcmd()
{
	gboolean res;
	char	buf[1024];
	char 	*cmd;

	cmd = "who";
	cmd = "who | awk '{print $1}'";
	res = sys_run_cmd(cmd, 1000*1000, "test", buf, sizeof(buf));
	if (res)
		printf("%s\n", buf);
	else
		printf("error : %s\n", cmd);

	cmd = "avahi-browse -arp";
	res = sys_run_cmd(cmd, 1000*1000, "test", buf, sizeof(buf));
	if (res)
		printf("%s\n", buf);
	else
		printf("error : %s\n", cmd);

	cmd = "xyz-123";
	res = sys_run_cmd(cmd, 1000*1000, "test", buf, sizeof(buf));
	if (res)
		printf("%s\n", buf);
	else
		printf("error : %s\n", cmd);

	cmd = "ls -l /";
	res = sys_run_cmd(cmd, 1000*1000, "test", buf, sizeof(buf));
	if (res)
		printf("%s\n", buf);
	else
		printf("error : %s\n", cmd);

	cmd = "bluetoothctl";
	res = sys_run_cmd(cmd, 1000*1000, "test", buf, sizeof(buf));
	if (res)
		printf("%s\n", buf);
	else
		printf("error : %s\n", cmd);

}
*/

gboolean g_run_cmd_get_output(gchar *cmd, char *caller, char *out, int size)
{
	gboolean res = FALSE;
	char	*out_buf = NULL;
	char	*err_buf = NULL;
	int		exit;
	int		cnt;

	if (out == NULL)
		size = 0;
	else
		*out = 0;

	if (c_strlen(cmd, 256) > 0) {	// check null or empty
		res = g_spawn_command_line_sync(cmd, &out_buf, &err_buf, &exit, NULL);
		if (res == FALSE) {
			grac_log_debug("%s : %s : can't run - %s", caller, __FUNCTION__, cmd);
		}
		else {
			if (exit != 0) {
				res = FALSE;
			}
			if (res && size > 1)
				c_strcpy(out, out_buf, size);

			// log stdout
			cnt = c_strlen(out_buf, -1);
			if (cnt > 0) {
				char *line, *ptr;
				line = out_buf;
				while (1) {
					if (*line == 0)
						break;
					ptr = c_strchr(line, '\n', -1);
					if (ptr != NULL)
						*ptr = 0;
					grac_log_debug("%s : %s - %s", caller, __FUNCTION__, line);
					if (ptr == NULL)
						break;
					*ptr = '\n';
					line = ptr + 1;
				}
			}

			// log stderr
			cnt = c_strlen(err_buf, -1);
			if (cnt > 0) {
				char *line, *ptr;
				line = err_buf;
				while (1) {
					if (*line == 0)
						break;
					ptr = c_strchr(line, '\n', -1);
					if (ptr != NULL)
						*ptr = 0;
					grac_log_debug("%s : %s - stderr - %s", caller, __FUNCTION__, line);
					if (ptr == NULL)
						break;
					*ptr = '\n';
					line = ptr + 1;
				}
			}

			if (out_buf)
				g_free(out_buf);
			if (err_buf)
				g_free(err_buf);
		}
	}

	return res;
}

void t_sys_run_cmd_buffer()
{
	char buf[1024];
	char output[1024];
	gboolean res;

	while (1) {
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		if (strlen(buf) <= 1)
			break;
		res = sys_run_cmd_get_output(buf, "test", output, sizeof(output));
		if (res)
			printf("%s\n", output);
		else
			printf("error\n");

		res = sys_run_cmd_no_output(buf, "test2");
		if (res)
			printf("OK\n");
		else
			printf("error\n");
	}
}
