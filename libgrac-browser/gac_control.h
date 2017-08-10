/*
 * grac_control.h
 *
 *  Created on: 2016. 6. 14.
 *      Author: yang
 */

#ifndef _GRAC_CONTROL_H_
#define _GRAC_CONTROL_H_

#include "grac_config.h"
#include "sys_comm.h"

typedef struct _GracInstanceInfo {
	char	instance_key[256];	// URL if browser
	char 	role_name[256];

	int		acc_printer;		// 0:delay 1:allow
	int		acc_network;		// 0:delay 1:allow
	int		acc_usb;				// 0:delay 1:allow
	int		acc_capture;		// 0:delay 1:allow
	int		acc_homedir;		// 0:delay 1:allow

} GracInstanceInfo;

//-----------------------------------------------------------------------
// common
//-----------------------------------------------------------------------

typedef struct _GracControl GracControl;

GracControl* grac_control_alloc (char *sender, char *session);
void	  grac_control_free(GracControl **pctrl);

gboolean grac_control_open(GracControl *ctrl);
void     grac_control_close(GracControl *ctrl);

char*    grac_control_get_error(GracControl *ctrl);
char*    grac_control_get_session(GracControl *ctrl);


// instance key is URL
// return : TRUE / FALSE
gboolean grac_control_app_executeL(GracControl *ctrl, char *instance, char *path, ...);		// the last argument should be NULL
gboolean grac_control_app_executeV(GracControl *ctrl, char *instance, char *path, int cnt, char*argv[]);

gboolean grac_control_app_change_instance(GracControl *ctrl, char *cur_instance, char *new_instance);
gboolean grac_control_app_get_instance_info(GracControl *ctrl, char *instance, GracInstanceInfo *info);

#endif /* _GRAC_CONTROL_H_ */
