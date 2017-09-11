/*
 * grac_rule_hook.h
 *
 *  Created on: 2017. 8. 16.
 *      Author: user
 */

#ifndef _grac_rule_hook_H_
#define _grac_rule_hook_H_

#include <glib.h>

gboolean grac_rule_hook_init();
gboolean grac_rule_hook_clear();

gboolean grac_rule_hook_allow_clipboard(gboolean allow);
gboolean grac_rule_hook_allow_screenshooter(gboolean allow);

#endif /* _grac_rule_hook_H_ */
