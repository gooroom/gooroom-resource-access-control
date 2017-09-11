/*
 * grac_rule_udev.h
 *
 *  Created on: 2017. 8. 16.
 *      Author: user
 */

#ifndef _grac_rule_udev_H_
#define _grac_rule_udev_H_

#include <glib.h>

#include "grac_rule.h"

typedef struct _GracRuleUdev GracRuleUdev;

GracRuleUdev* grac_rule_udev_alloc(const char *map_path);
void grac_rule_udev_free(GracRuleUdev **pRule);

gboolean grac_rule_udev_make_rules(GracRuleUdev *grac_udev, GracRule* grac_rule, const char *udev_rule_path);
gboolean grac_rule_udev_make_empty(GracRuleUdev *grac_udev, const char *udev_rule_path);


#endif /* _grac_rule_udev_H_ */
