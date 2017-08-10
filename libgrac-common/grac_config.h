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
const gint  grac_config_max_length_packet();
const gint  grac_config_max_length_url();

const gchar*	grac_config_data_base_directory(gboolean end_mark);		// end_mark "/"
const gchar*	grac_config_path_access();
const gchar*	grac_config_path_url_list();

const gchar*	grac_config_path_browser_app();

#endif /* GRAC_CONFIG_H_ */
