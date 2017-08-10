/*
 * grac_url_list.h
 *
 *  Created on: 2016. 9. 30.
 *      Author: yang
 */

#ifndef _grac_url_list_H_
#define _grac_url_list_H_

#include <glib.h>

#include "grac_url.h"

typedef struct _GracUrlList GracUrlList;

GracUrlList* grac_url_list_alloc();
void     grac_url_list_free(GracUrlList **plist);
void     grac_url_list_clear(GracUrlList *list);
GracUrlList* grac_url_list_alloc_copy(GracUrlList *src);

gboolean grac_url_list_load (GracUrlList *list, char *path, gboolean blacklist);  // if path is NULL, path is default
gboolean grac_url_list_save (GracUrlList *list, char *path);

void	grac_url_list_set_whitelist(GracUrlList *list);			// default
void	grac_url_list_set_blacklist(GracUrlList *list);
gboolean grac_url_list_is_blacklist(GracUrlList *list);

gboolean grac_url_list_is_allowed(GracUrlList *list, gchar *url_str);

int	grac_url_list_get_count(GracUrlList *list);
GracUrl *grac_url_list_get_url(GracUrlList *list, int idx);

int	grac_url_list_find_match(GracUrlList *list, gchar *url_str);

#endif /* _grac_url_list_H_ */
