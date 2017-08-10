/*
 * grac_map.h
 *
 *  Created on: 2016. 6. 30.
 *      Author: yang
 */

#ifndef _GRAC_MAP_H_
#define _GRAC_MAP_H_

#include <glib.h>

typedef struct _GracMap GracMap;

GracMap* grac_map_alloc();
void	 grac_map_free(GracMap **pmap);
void grac_map_clear(GracMap* map);


gboolean grac_map_set_data(GracMap *map, const char *key, const char *data);
gboolean grac_map_del_data(GracMap *map, const char *key);

const char* grac_map_get_data(GracMap *map, const char *key); // if error, return NULL
int  grac_map_get_data_len(GracMap *map, const char *key);    // including the end of line character 0, if error, return 0

const char* grac_map_first_key(GracMap *map);  // if no key, return NULL
const char* grac_map_next_key (GracMap *map);  // if no more key, return NULL


#endif /* _GRAC_MAP_H_ */
