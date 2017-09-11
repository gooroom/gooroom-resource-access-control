/*
 * grac_rule.h
 *
 *  Created on: 2016. 8. 3.
 *      Author: yang
 */

#ifndef _GRAC_RULE_H_
#define _GRAC_RULE_H_

#include <glib.h>

#include "grac_resource.h"

typedef struct _GracRule GracRule;

GracRule* grac_rule_alloc();
void grac_rule_free(GracRule **pRule);
void grac_rule_clear(GracRule *acc);

gboolean grac_rule_load  (GracRule *data, gchar* path);
gboolean grac_rule_apply (GracRule *data);
gboolean grac_rule_apply_allow_all (GracRule *data);

int grac_rule_get_perm_id(GracRule *rule, int res_id);	// if error, return -1

gboolean grac_rule_set_res_perm_by_name(GracRule *rule, gchar* res_name, gchar* perm_str);  // if no key, add
gboolean grac_rule_set_res_perm_by_id  (GracRule *rule, int res_id, int perm_id);  // if no key, add

gboolean grac_rule_set_default_of_guest(GracRule* rule);
gboolean grac_rule_set_default_of_admin(GracRule* rule);	// all enable


//gboolean grac_rule_apply_by_user (GracRule *data, gchar* username);

// need data handling for editor
// --- working ----- 향후 삭제
//gboolean grac_rule_find_attr(GracRule *rule, gchar* attr);
//gboolean grac_rule_del_attr (GracRule *rule, gchar* attr);
//gchar*   grac_rule_get_attr (GracRule *rule, gchar* attr);
// --- working -----

#endif /* _GRAC_RULE_H_ */
