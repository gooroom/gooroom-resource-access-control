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
 * grac_resource.h
 *
 *  Created on: 2017. 8. 16.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_DEVICE_GRAC_RESOURCE_H_
#define LIBGRAC_DEVICE_GRAC_RESOURCE_H_

#include <glib.h>

// resource id
#define GRAC_RESOURCE_NETWORK		 10
#define GRAC_RESOURCE_PRINTER		 20
#define GRAC_RESOURCE_BLUETOOTH	 30
#define GRAC_RESOURCE_USB				 40
#define GRAC_RESOURCE_KEYBOARD	 50
#define GRAC_RESOURCE_MOUSE			 60

#define GRAC_RESOURCE_USB_MEMORY 100
#define GRAC_RESOURCE_CD_DVD		 110

#define GRAC_RESOURCE_S_CAPTURE	 200
#define GRAC_RESOURCE_CLIPBOARD	 210

#define GRAC_RESOURCE_MICROPHONE 300
#define GRAC_RESOURCE_SOUND			 310
#define GRAC_RESOURCE_CAMERA		 320
#define GRAC_RESOURCE_WIRELESS	 330
#define GRAC_RESOURCE_EXT_LAN		 340

#define GRAC_RESOURCE_OTHERS		 999

// permission id
#define GRAC_PERMISSION_DISALLOW	0
#define GRAC_PERMISSION_ALLOW			1
#define GRAC_PERMISSION_READONLY	2
#define GRAC_PERMISSION_READWRITE	3


char* grac_resource_get_resource_name(int res_id);
char* grac_resource_get_permission_name(int perm_id);

int		grac_resource_get_resource_id(char* res_name);
int		grac_resource_get_permission_id(char* perm_name);

char* grac_resource_find_first_resource();
char* grac_resource_find_next_resource();

char* grac_resource_find_first_permission(int res_id);
char* grac_resource_find_next_permission();

gboolean grac_resource_is_kind_of_storage(int res_id);

int	grac_resource_first_control_res_id();
int	grac_resource_next_control_res_id();


#endif // LIBGRAC_DEVICE_GRAC_RESOURCE_H_
