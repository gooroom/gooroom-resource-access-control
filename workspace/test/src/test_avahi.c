/*
 * test_avahi.c
 *
 *  Created on: 2017. 11. 1.
 *      Author: yang
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


void t_avahi()
{
	if (grac_printer_init() == FALSE) {
		printf("Init error\n");
		return;
	}


	char	buf[256];
	while (1) {
		if (fgets(buf, 256, stdin) == NULL)
			break;
		c_strtrim(buf, 256);
		if (c_strmatch(buf, "allow")) {
			printf("allow\n");
			grac_printer_apply(TRUE);
		}
		else if (c_strmatch(buf, "disallow")) {
			printf("disallow\n");
			grac_printer_apply(FALSE);
		}
		else if (c_strmatch(buf, "exit")) {
			printf("exit\n");
			break;
		}
		else {
			printf("loop\n");
		}
	}

	if (grac_printer_end() == FALSE) {
		printf("Ending error\n");
		return;
	}
}


void test_list_by_cups()
{
	cups_dest_t *printer_dests = NULL;
	int printer_count = 0;

	// printer_count = cupsGetDests2(CUPS_HTTP_DEFAULT, &printer_dests); same results
	printer_count = cupsGetDests(&printer_dests);
	if (printer_count == 0) {
		printf("Not found printer\n");
		return;
	}

	int idx;
	for (idx=0; idx < printer_count; idx++)
		printf("%d : %s\n", idx, printer_dests[idx].name);

	if (printer_dests != NULL) {
			cupsFreeDests(printer_count, printer_dests);
			printer_dests = NULL;
			printer_count = 0;
	}

	printf("\n");
	test_list_by_cups();

}


