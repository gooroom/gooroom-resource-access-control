/*
 * grac_rule_network_network.h
 *
 *  Created on: 2016. 8. 3.
 *      Author: yang
 */

#ifndef _GRAC_RULE_NETWORK_H_
#define _GRAC_RULE_NETWORK_H_

#include <glib.h>

typedef struct _GracRuleNetwork GracRuleNetwork;

typedef enum {
	GRAC_NETWORK_DIR_INBOUND = 1,
	GRAC_NETWORK_DIR_OUTBOUND,
	GRAC_NETWORK_DIR_ALL
} grac_network_dir_t;

// return value of get function
//	-1: error
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
int			 grac_rule_network_get_protocol(GracRuleNetwork *rule, int idx, char **protocol);

gboolean grac_rule_network_add_src_port(GracRuleNetwork *rule, char *port);	// port : single or range
int			 grac_rule_network_src_port_count(GracRuleNetwork *rule);
int			 grac_rule_network_get_src_port(GracRuleNetwork *rule, int idx, char **port); // port : single or range

gboolean grac_rule_network_add_dst_port(GracRuleNetwork *rule, char *port);	// port : single or range
int			 grac_rule_network_dst_port_count(GracRuleNetwork *rule);
int			 grac_rule_network_get_dst_port(GracRuleNetwork *rule, int idx, char **port); // port : single or range

#endif /* _GRAC_RULE_NETWORK_H_ */
