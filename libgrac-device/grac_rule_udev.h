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
 * grac_rule_udev.h
 *
 *  Created on: 2017. 8. 16.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_DEVICE_GRAC_RULE_UDEV_H_
#define LIBGRAC_DEVICE_GRAC_RULE_UDEV_H_

#include <glib.h>

#include "grac_rule.h"

typedef struct _GracRuleUdev GracRuleUdev;

GracRuleUdev* grac_rule_udev_alloc(const char *map_path);
void grac_rule_udev_free(GracRuleUdev **pRule);

gboolean grac_rule_udev_make_rules(GracRuleUdev *grac_udev, GracRule* grac_rule, const char *udev_rule_path);
gboolean grac_rule_udev_make_empty(GracRuleUdev *grac_udev, const char *udev_rule_path);


#endif // LIBGRAC_DEVICE_GRAC_RULE_UDEV_H_
