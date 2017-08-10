/*
 * grac_url.h
 *
 *  Created on: 2016. 6. 16.
 *      Author: yang
 */

#ifndef _GRAC_URL_H_
#define _GRAC_URL_H_

#include <glib.h>

typedef struct _GracUrl GracUrl;

GracUrl* grac_url_alloc();
GracUrl* grac_url_alloc_copy(GracUrl* org_url);
void    grac_url_init(GracUrl *url);
void    grac_url_free(GracUrl **purl);

gboolean grac_url_set_pattern(GracUrl *url, char *pattern);
char*    grac_url_get_pattern(GracUrl *url);

gboolean grac_url_is_match(GracUrl *url, char *url_str);


#endif /* _GRAC_URL_H_ */
