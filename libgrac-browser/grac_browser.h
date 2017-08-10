/*
 * grac_browser.h
 *
 *  Created on: 2017. 7. 3.
 *      Author: yang
 */

#ifndef _GRAC_BROWSER_H_
#define _GRAC_BROWSER_H_

#include "grac_config.h"
#include "sys_comm.h"

//-----------------------------------------------------------------------
// common
//-----------------------------------------------------------------------

typedef struct _GracBrowser GracBrowser;

GracBrowser* grac_browser_alloc ();
void	  grac_browser_free(GracBrowser **pctrl);

gboolean grac_browser_exec(GracBrowser *ctrl, char *url);
gboolean grac_browser_is_trusted(GracBrowser *ctrl, char *url);

gboolean grac_browser_executeL(GracBrowser *ctrl, char *url, char *path, ...);		// the last argument should be NULL
gboolean grac_browser_executeV(GracBrowser *ctrl, char *url, char *path, int cnt, char*argv[]);  // argv has no path


#endif /* _GRAC_BROWSER_H_ */
