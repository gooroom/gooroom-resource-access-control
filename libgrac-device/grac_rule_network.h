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
 * grac_rule_network.h
 *
 *  Created on: 2016. 8. 3.
 *      Author: gooroom@gooroom.kr
 */

#ifndef LIBGRAC_DEVICE_GRAC_RULE_NETWORK_H_
#define LIBGRAC_DEVICE_GRAC_RULE_NETWORK_H_

#include <glib.h>

typedef struct _GracRuleNetwork GracRuleNetwork;

typedef enum {
	GRAC_NETWORK_DIR_INBOUND = 1,
	GRAC_NETWORK_DIR_OUTBOUND,
	GRAC_NETWORK_DIR_ALL
} grac_network_dir_t;

// return value of get function
// 	-1: error
//   0: not setting
//   1: OK

GracRuleNetwork* grac_rule_network_alloc();
void grac_rule_network_free(GracRuleNetwork **pRule);
void grac_rule_network_clear(GracRuleNetwork *rule);

gboolean grac_rule_network_set_allow(GracRuleNetwork *rule, gboolean allow);
int			 grac_rule_network_get_allow(GracRuleNetwork *rule, gboolean *allow);

// address : ipv4 or ipv6, can be with mask
gboolean grac_rule_network_set_address(GracRuleNetwork *rule, char *address);
int			 grac_rule_network_get_address(GracRuleNetwork *rule, char **address);
int			 grac_rule_network_get_address_ex(GracRuleNetwork *rule, char **addr, int *mask, gboolean *ipv6);

gboolean grac_rule_network_set_direction(GracRuleNetwork *rule, grac_network_dir_t direction);
int			 grac_rule_network_get_direction(GracRuleNetwork *rule, grac_network_dir_t* direction);

gboolean grac_rule_network_set_mac(GracRuleNetwork *rule, char *mac);
int			 grac_rule_network_get_mac(GracRuleNetwork *rule, char **mac);

gboolean grac_rule_network_add_protocol(GracRuleNetwork *rule, char *protocol);
int			 grac_rule_network_protocol_count(GracRuleNetwork *rule);
int			 grac_rule_network_get_protocol(GracRuleNetwork *rule, int proto_idx, char **protocol);
int			 grac_rule_network_find_protocol(GracRuleNetwork *rule, char *protocol);  // return idx */

gboolean grac_rule_network_add_src_port(GracRuleNetwork *rule, int ptoto_idx, char *port);	// port : single or range
int			 grac_rule_network_src_port_count(GracRuleNetwork *rule, int ptoto_idx);
int			 grac_rule_network_get_src_port(GracRuleNetwork *rule, int ptoto_idx, int port_idx, char **port); // port : single or range

gboolean grac_rule_network_add_dst_port(GracRuleNetwork *rule, int ptoto_idx, char *port);	// port : single or range
int			 grac_rule_network_dst_port_count(GracRuleNetwork *rule, int ptoto_idx);
int			 grac_rule_network_get_dst_port(GracRuleNetwork *rule, int ptoto_idx, int port_idx, char **port); // port : single or range

#endif // LIBGRAC_DEVICE_GRAC_RULE_NETWORK_H_
