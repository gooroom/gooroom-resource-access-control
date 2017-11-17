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
 * grac_adjust_udev_rule.h
 *
 *  Created on: 2017. 9. 20.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_DEVICE_GRAC_ADJUST_UDEV_RULE_H_
#define LIBGRAC_DEVICE_GRAC_ADJUST_UDEV_RULE_H_

#include <glib.h>

gboolean	grac_adjust_udev_rule_file(char *org_path, char *adj_path);

#endif // LIBGRAC_DEVICE_GRAC_ADJUST_UDEV_RULE_H_
