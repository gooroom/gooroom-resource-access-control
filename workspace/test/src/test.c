/*
 ============================================================================
 Name        : test.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <cups/cups.h>

#include "grac_log.h"
#include "grac_config.h"
#include "grac_rule.h"
#include "grac_resource.h"
//#include "grac_block_device.h"
#include "cutility.h"
#include "sys_user.h"
#include "sys_file.h"
#include "sys_etc.h"

#include <glib.h>
#include <glib/gstdio.h>



void t_glib()
{
	char *name = g_file_read_link("/proc/self/exe", NULL);
	if (name) {
		printf("%s\n", name);
		free(name);
	}
}



int main(int argc, char *argv[])
{
	gboolean res;
	char	*cmd = "who | awk '{printf $1}'";
	char	buf[1024];

	res = sys_run_cmd_get_output(cmd, "test", buf, sizeof(buf));
	if (res)
		printf("%s\n", buf);
	else
		printf("error\n");

	cmd = "ls 2>&1";
	res = sys_run_cmd_get_output(cmd, "test", buf, sizeof(buf));
	if (res)
		printf("%s\n", buf);
	else
		printf("error\n");

	return EXIT_SUCCESS;
}



