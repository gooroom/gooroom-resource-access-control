/*
 * grac_log.h
 *
 *  Created on: 2016. 5. 17.
 *      Author: user
 */

#ifndef _GRM_LOG_H_
#define _GRM_LOG_H_

#include <glib.h>

void grac_log_set_name (gchar *appname);

void grac_log_debug  (gchar *format, ...);
void grac_log_info   (gchar *format, ...);
void grac_log_notice (gchar *format, ...);
void grac_log_warning(gchar *format, ...);
void grac_log_error  (gchar *format, ...);

#endif
