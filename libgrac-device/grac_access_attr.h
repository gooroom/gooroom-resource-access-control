/*
 * grac_access_attr.h
 *
 *  Created on: 2016. 8. 17.
 *      Author: yang
 */

#ifndef _GRAC_ACCESS_ATTR_H_
#define _GRAC_ACCESS_ATTR_H_

#include <glib.h>

char* grac_access_attr_find_first_attr();
char* grac_access_attr_find_next_attr ();
char* grac_access_attr_find_first_perm(gchar* attr);
char* grac_access_attr_find_next_perm ();

int	  grac_access_attr_get_perm_value(gchar* attr, gchar *perm);
int	  grac_access_attr_get_attr_value(gchar* attr);

char* grac_access_attr_get_attr_name(int target);
char* grac_access_attr_get_perm_name(int target, int perm_value);


#endif /* _GRAC_ACCESS_ATTR_H_ */
