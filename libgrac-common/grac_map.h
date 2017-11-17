/*
 * Copyright (c) 2015 - 2017 gooroom <gooroom@gooroom.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * grac_map.h
 *
 *  Created on: 2016. 6. 30.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_COMMON_GRAC_MAP_H_
#define LIBGRAC_COMMON_GRAC_MAP_H_

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
const char* grac_map_next_key(GracMap *map);  // if no more key, return NULL


#endif // LIBGRAC_COMMON_GRAC_MAP_H_
