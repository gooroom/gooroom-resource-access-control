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
