/*
 * grac_config.h
 *
 *  Created on: 2016. 6. 13.
 *      Author: yang
 */

#ifndef GRAC_CONFIG_H_
#define GRAC_CONFIG_H_

#include <glib.h>

const gint	grac_config_max_length_path();
const gint  grac_config_max_length_username();

const gchar*	grac_config_dir_grac_data();

const gchar*	grac_config_path_grac_rules(char* login_name);
const gchar*	grac_config_path_default_grac_rules();
const gchar*	grac_config_path_user_grac_rules();
const gchar*	grac_config_path_udev_rules();
const gchar*	grac_config_path_udev_map();

const gchar*	grac_config_path_hook_screenshooter_so();
const gchar*	grac_config_path_hook_screenshooter_conf();

const gchar*	grac_config_path_hook_clipboard_so();
const gchar*	grac_config_path_hook_clipboard_conf();

const gchar*	grac_config_file_ld_so_preload();
const gchar*	grac_config_path_ld_so_preload();
const gchar*	grac_config_dir_ld_so_preload();


#endif /* GRAC_CONFIG_H_ */
