/*
 * Copyright (c) 2015 - 2017 gooroom <gooroom@gooroom.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * grac_log.h
 *
 *  Created on: 2017. 5. 17.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_COMMON_GRAC_LOG_H_
#define LIBGRAC_COMMON_GRAC_LOG_H_

#include <glib.h>

void grac_log_set_name(gchar *appname);

void grac_log_debug(gchar *format, ...);
void grac_log_info(gchar *format, ...);
void grac_log_notice(gchar *format, ...);
void grac_log_warning(gchar *format, ...);
void grac_log_error(gchar *format, ...);

#endif  // LIBGRAC_COMMON_GRAC_LOG_H_
