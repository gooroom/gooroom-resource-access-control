/*
 * test_grac.c
 *
 *  Created on: 2017. 9. 14.
 *      Author: yang
 */

#include <stdio.h>

#include "grac_config.h"
#include "grac_rule.h"
#include "grac_resource.h"
#include "cutility.h"
#include "sys_user.h"


void test_resource()
{
	char	*res_name;
	int		res_id;
	char *perm_name;

	res_name = grac_resource_find_first_resource();
	while (res_name) {
		res_id = grac_resource_get_resource_id(res_name);
		printf("%-10s\t--> ", res_name);
		perm_name = grac_resource_find_first_permission(res_id);
		while (perm_name) {
			printf("%s\t", perm_name);
			perm_name = grac_resource_find_next_permission();
		}
		printf("\n");
		res_name = grac_resource_find_next_resource();
	}
}

void test_grac_rule_load()
{
	GracRule	*grac_rule = grac_rule_alloc();

//	grac_rule_load(grac_rule, NULL);
	grac_rule_load(grac_rule, "test-grac.rules");

	grac_rule_dump(grac_rule, NULL);

	grac_rule_free(&grac_rule);
}

void test_grac_load_apply()
{
	const char	*rule_path;

	rule_path = grac_config_path_default_grac_rules();

	GracRule *grac_rule = grac_rule_alloc();
	if (rule_path != NULL) {
		gboolean res;
		res = grac_rule_load(grac_rule, (gchar*)rule_path);
		if (res == FALSE) {
			printf("%s() : load rule error : %s", __FUNCTION__, rule_path);
			return;
		}
		else {
			grac_rule_apply(grac_rule);
		}
	}

}
