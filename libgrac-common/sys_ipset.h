/*
 * sys_ipset.h
 *
 *  Created on: 2017. 7. 13.
 *      Author: user
 */

#ifndef _SYS_IPSET_H_
#define _SYS_IPSET_H_

#include <glib.h>

#include "sys_ipt.h"

typedef struct _sys_ipset_entry_t sys_ipset_entry_t;

#define SYS_IPSET_TYPE_IP_ONLY  1
#define SYS_IPSET_TYPE_IP_PORT	2

//------------------------
// set handling
//------------------------

gboolean sys_ipset_create(char *name, int type, gboolean ipv6);
gboolean sys_ipset_delete(char *name);
gboolean sys_ipset_clear (char *name);

gboolean sys_ipset_add_entry   (char *name, sys_ipset_entry_t *entry);
//gboolean sys_ipset_delete_entry(char *name, sys_ipset_entry_t *entry);

//----------------------------------------
// setting entry of set
//----------------------------------------

void sys_ipset_entry_init(sys_ipset_entry_t *entry);
gboolean sys_ipset_entry_get_type(sys_ipset_entry_t *entry, int *type, gboolean *ipv6);

gboolean sys_ipset_entry_set_addr (sys_ipset_entry_t *entry, guint8  addr[4]);
gboolean sys_ipset_entry_set_addr6(sys_ipset_entry_t *entry, guint16 addr[8]);
gboolean sys_ipset_entry_set_addr_str (sys_ipset_entry_t *entry, gchar *addr_str);
gboolean sys_ipset_entry_set_addr6_str(sys_ipset_entry_t *entry, gchar *addr_str);

gboolean sys_ipset_entry_set_mask (sys_ipset_entry_t *entry, guchar  mask);

gboolean sys_ipset_entry_set_protocol(sys_ipset_entry_t *entry, int protocol);
gboolean sys_ipset_entry_set_protocol_name(sys_ipset_entry_t *entry, char *proto_name);

gboolean sys_ipset_entry_set_port (sys_ipset_entry_t *entry, int port);
gboolean sys_ipset_entry_set_port2(sys_ipset_entry_t *entry, int from_port, int to_port);
gboolean sys_ipset_entry_set_port_name (sys_ipset_entry_t *entry, gchar *name);
gboolean sys_ipset_entry_set_port2_name(sys_ipset_entry_t *entry, gchar *from_name, gchar *to_name);

#endif /* _SYS_IPSET_H_ */
