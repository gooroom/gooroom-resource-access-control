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

#include <cups/cups.h>

#include "grm_log.h"
#include "grac_config.h"
#include "grac_rule.h"
#include "grac_resource.h"
//#include "grac_block_device.h"
#include "cutility.h"
#include "sys_user.h"
#include "sys_file.h"
#include "sys_etc.h"


int main(int argc, char *argv[])
{

//printf("check - %d\n", (int) grac_udev_check_existing("sda"));

/*
	if (argc > 1) {
		grac_eject(argv[1]);
	}
*/
//	grac_eject("vda");
//	printf("\n");
//	grac_eject("vda1");

// 	t_block();

//	test_udev();

	return EXIT_SUCCESS;
}



