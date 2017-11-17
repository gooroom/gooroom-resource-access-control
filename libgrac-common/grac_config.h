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
 * grac_config.h
 *
 *  Created on: 2016. 6. 13.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_COMMON_GRAC_CONFIG_H_
#define LIBGRAC_COMMON_GRAC_CONFIG_H_

#include <glib.h>

const gint	grac_config_max_length_path();
const gint  grac_config_max_length_username();

const gchar*	grac_config_dir_grac_data();

const gchar*	grac_config_path_grac_rules(char* login_name);
const gchar*	grac_config_path_default_grac_rules();
const gchar*	grac_config_path_user_grac_rules();

const gchar*	grac_config_file_udev_rules();
const gchar*	grac_config_path_udev_rules();

const gchar*	grac_config_path_udev_map_org();
const gchar*	grac_config_path_udev_map_local();

const gchar*	grac_config_path_hook_screenshooter_so();
const gchar*	grac_config_path_hook_screenshooter_conf();

const gchar*	grac_config_path_hook_clipboard_so();
const gchar*	grac_config_path_hook_clipboard_conf();

const gchar*	grac_config_file_ld_so_preload();
const gchar*	grac_config_path_ld_so_preload();
const gchar*	grac_config_dir_ld_so_preload();


const gchar*	grac_config_path_recover_info();

int	grac_config_network_printer_port();


#endif /* LIBGRAC_COMMON_GRAC_CONFIG_H_ */
