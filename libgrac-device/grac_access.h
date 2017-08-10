/*
 * grac_access.h
 *
 *  Created on: 2016. 8. 3.
 *      Author: yang
 */

#ifndef _GRAC_ACCESS_H_
#define _GRAC_ACCESS_H_

#include <glib.h>

#define GRAC_ATTR_PRINTER     		1
#define GRAC_ATTR_NETWORK     		2
#define GRAC_ATTR_SCREEN_CAPTURE 3
#define GRAC_ATTR_USB         		4
#define GRAC_ATTR_HOME_DIR     	5
#define GRAC_ATTR_ETC	        	99

#define GRAC_PERM_DENY 		0
#define GRAC_PERM_ALLOW  	1

typedef struct _GracAccess GracAccess;

GracAccess* grac_access_alloc();
void grac_access_free(GracAccess **pAccess);
void grac_access_clear(GracAccess *acc);

gboolean grac_access_load  (GracAccess *data, gchar* path);

gboolean grac_access_apply (GracAccess *data);
gboolean grac_access_apply_by_user (GracAccess *data, gchar* username);

gboolean grac_access_set_default_of_guest(GracAccess* acc);
gboolean grac_access_set_default_of_admin(GracAccess* acc);	// all enable

// need data handling for editor
// --- working ----- 향후 삭제
gboolean grac_access_get_allow(GracAccess *access, int target);
gboolean grac_access_find_attr(GracAccess *access, gchar* attr);

gboolean grac_access_put_attr (GracAccess *access, gchar* attr, gchar* value);  // if no key, add key
gboolean grac_access_set_attr (GracAccess *access, gchar* attr, gchar* value);  // if no key, return FALSE
gboolean grac_access_del_attr (GracAccess *access, gchar* attr);
gchar*   grac_access_get_attr (GracAccess *access, gchar* attr);

gchar* 	 grac_access_enum_attr_name(GracAccess *access, gboolean first);

gboolean grac_access_fill_attr(GracAccess *access);   // add omitted attributes
gboolean grac_access_init_attr(GracAccess *access);   // initialize all attributes
// --- working -----

#endif /* _GRAC_ACCESS_H_ */
