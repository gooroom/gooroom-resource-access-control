/*
 * sys_ipt.h
 *
 *  Created on: 2017. 7. 13.
 *      Author: user
 */

#ifndef _SYS_IPT_H_
#define _SYS_IPT_H_

#include <glib.h>

typedef struct _sys_ipt_rule sys_ipt_rule;

#define SYS_IPT_CHAIN_B_INPUT 	0x0001
#define SYS_IPT_CHAIN_B_OUTPUT	0x0002
#define SYS_IPT_CHAIN_B_INOUT		(SYS_IPT_CHAIN_B_INPUT|SYS_IPT_CHAIN_B_OUTPUT)
#define SYS_IPT_CHAIN_B_ALL		  0xffff

#define SYS_IPT_TARGET_ACCEPT   1
#define SYS_IPT_TARGET_DROP     2

#define SYS_IPT_PROTOCOL_ALL   -1
#define SYS_IPT_PROTOCOL_IP    0
#define SYS_IPT_PROTOCOL_TCP   6
#define SYS_IPT_PROTOCOL_UDP   17

//------------------------------------------------------------------------------------
// rule handling
//------------------------------------------------------------------------------------
sys_ipt_rule *sys_ipt_rule_alloc();
void sys_ipt_rule_free(sys_ipt_rule **prule);

void	sys_ipt_rule_init(sys_ipt_rule *rule);
gboolean	sys_ipt_rule_set_target  (sys_ipt_rule *rule, int target);
gboolean	sys_ipt_rule_set_chain   (sys_ipt_rule *rule, int chain);

gboolean	sys_ipt_rule_set_protocol(sys_ipt_rule *rule, int protocol);
gboolean	sys_ipt_rule_set_protocol_name(sys_ipt_rule *rule, char *proto_name);   // special name "all"

// set_port() is  valid for TCP or UDP
// error if no protocol is specified
//  if port is -1, the end of valid port number
gboolean	sys_ipt_rule_set_port (sys_ipt_rule *rule, int port);
gboolean	sys_ipt_rule_set_port2(sys_ipt_rule *rule, int from_port, int to_port);
gboolean	sys_ipt_rule_set_port_name (sys_ipt_rule *rule, gchar *name);
gboolean	sys_ipt_rule_set_port_name2(sys_ipt_rule *rule, gchar *from_name, gchar *to_name);

gboolean	sys_ipt_rule_set_port_str(sys_ipt_rule *rule, gchar *port_str);	// 80,  80-90,  80:90

gboolean	sys_ipt_rule_set_addr (sys_ipt_rule *rule, guint8 addr[4]);
gboolean	sys_ipt_rule_set_addr6(sys_ipt_rule *rule, guint16 addr[8]);
gboolean	sys_ipt_rule_set_addr_str (sys_ipt_rule *rule, gchar *addr_str);
gboolean	sys_ipt_rule_set_addr6_str(sys_ipt_rule *rule, gchar *addr_str);

gboolean	sys_ipt_rule_set_ipset   (sys_ipt_rule *rule, gchar* setname);
gboolean	sys_ipt_rule_set_ipset6  (sys_ipt_rule *rule, gchar* setname);

gboolean	sys_ipt_rule_set_mask    (sys_ipt_rule *rule, guchar mask);

gboolean	sys_ipt_rule_set_mac_addr(sys_ipt_rule *rule, guint8 mac[6]);
gboolean	sys_ipt_rule_set_mac_addr_str(sys_ipt_rule *rule, gchar *mac_str);

//------------------------------------------------------------------------------------
// control iptables
//------------------------------------------------------------------------------------
gboolean	sys_ipt_clear_all();

gboolean	sys_ipt_insert_rule(sys_ipt_rule *rule);
gboolean	sys_ipt_append_rule(sys_ipt_rule *rule);

gboolean	sys_ipt_insert_all_drop_rule();
gboolean	sys_ipt_append_all_drop_rule();
gboolean	sys_ipt_insert_all_accept_rule();
gboolean	sys_ipt_append_all_accept_rule();


#endif /* _SYS_IPT_H_ */
