/*
 * gac_access_list.h
 *
 *  Created on: 2017. 7. 18.
 *      Author: yang
 */


#ifndef _GAC_ACCESS_LIST_H_
#define _GAC_ACCESS_LIST_H_

#include <glib.h>

#include "gac_access.h"

typedef struct _GacAccessList GacAccessList;

GacAccessList* gac_access_list_alloc();
void     gac_access_list_free(GacAccessList **pdata);
void     gac_access_list_clear(GacAccessList *data);

GacAccessList* gac_access_list_alloc_copy(GacAccessList *src);

gboolean gac_access_list_load (GacAccessList *data, gchar *path);		// if path is NULL, the default path is used.
gboolean gac_access_list_save (GacAccessList *data, gchar *path);		// if path is NULL, the default path is used.

GacAccess* gac_access_list_default_access_of_guest(GacAccessList* data);
GacAccess* gac_access_list_default_access_of_admin(GacAccessList* data);

gboolean gac_access_list_put_access_value(GacAccessList *data, gchar *key, gchar *value);   // if no key, add key
gboolean gac_access_list_set_access_value(GacAccessList *data, gchar *key, gchar *value);   // if no key, return error
gchar*   gac_access_list_get_access_value(GacAccessList *data, gchar *key);	// should not free the return value

GacAccess* gac_access_list_get_access(GacAccessList *data);


#endif /* _GAC_ACCESS_LIST_H_ */
