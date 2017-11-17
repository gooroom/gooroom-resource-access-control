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
 * grac_rule.h
 *
 *  Created on: 2016. 8. 3.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_DEVICE_GRAC_RULE_H_
#define LIBGRAC_DEVICE_GRAC_RULE_H_

#include <stdio.h>
#include <glib.h>

#include "grac_resource.h"

typedef struct _GracRule GracRule;

GracRule* grac_rule_alloc();
void grac_rule_free(GracRule **pRule);
void grac_rule_clear(GracRule *acc);

gboolean grac_rule_load(GracRule *data, gchar* path);
gboolean grac_rule_apply(GracRule *data);
gboolean grac_rule_apply_allow_all(GracRule *data);

int grac_rule_get_perm_id(GracRule *rule, int res_id);	// if error, return -1

gboolean grac_rule_set_perm_by_name(GracRule *rule, gchar* res_name, gchar* perm_str);  // if no key, add
gboolean grac_rule_set_perm_by_id(GracRule *rule, int res_id, int perm_id);  // if no key, add

int		   grac_rule_bluetooth_mac_count(GracRule *rule);
gboolean grac_rule_bluetooth_mac_get_addr(GracRule *rule, int idx, char *addr, int addr_size);

gboolean grac_rule_set_default_of_guest(GracRule* rule);
gboolean grac_rule_set_default_of_admin(GracRule* rule);	// all enable

void grac_rule_dump(GracRule *rule, FILE *out_fp);		// only for debugging

gboolean grac_rule_pre_process();
gboolean grac_rule_post_process();


#endif // LIBGRAC_DEVICE_GRAC_RULE_H_
