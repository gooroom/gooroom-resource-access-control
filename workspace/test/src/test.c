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

#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <cups/cups.h>

#include "grm_log.h"
#include "grac_config.h"
#include "grac_rule.h"
#include "grac_resource.h"
#include "grac_printer.h"
#include "cutility.h"
#include "sys_user.h"
#include "sys_etc.h"


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

int main()
{
	char	name[256];

//t_runcmd();
	sys_user_get_login_name(name, sizeof(name));
	printf("%s\n", name);


	return EXIT_SUCCESS;
}




