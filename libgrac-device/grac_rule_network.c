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
 * grac_rule_network.c
 *
 *  Created on: 2016. 8. 3.
 *      Author: gooroom@gooroom.kr
 */

/**
  @file 	 	grac_rule_network.c
  @brief		네트워크 접근 권한 정보
  @details	class GracRuleNetwork 정의 (GTK style의 c언어를 이용한 객체처리 사용) \n
  				헤더파일 :  	grac_rule_network.h	\n
  				라이브러리:	libgrac-device.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "grac_rule_network.h"
#include "grac_map.h"
#include "grac_log.h"
#include "cutility.h"

typedef struct _GracRuleNetwork  GracRuleNetwork;
typedef struct _GracRuleProtocol GracRuleProtocol;

struct _GracRuleProtocol {
	gchar		  *protocol_name;
	GPtrArray *src_port_array;			// char*, single or range "00"  or "00-10"
	gint		   src_port_count;

	GPtrArray *dst_port_array;			// char*, single or range "00"  or "00-10"
	gint		   dst_port_count;
};


struct _GracRuleNetwork {
	struct _perm {
		gboolean allow;
		gboolean set;		// FALSE: not setting
	} perm;

	struct _addr {
		gchar		  *full_addr;		// NULL : not setting
		gchar		  *part_addr;		// NULL : not setting
		guint			part_mask;		// 0:not setting
		gint		  kind;					// 0:not set 4:ipv4, 6:ipv6
	} addr;

	gchar		*mac_addr_str;	// NULL : not setting

	struct _dir {
		grac_network_dir_t dir;
		gboolean set;		// FALSE: not setting
	} dir;

	GPtrArray *protocols_array;	// struct protocol*
	gint		   protocols_count;
};

GracRuleProtocol* grac_rule_protocol_alloc()
{
	GracRuleProtocol* rule = malloc(sizeof(GracRuleProtocol));
	if (rule) {
		c_memset(rule, 0, sizeof(GracRuleProtocol));
	}
	return rule;
}

void grac_rule_protocol_clear(GracRuleProtocol *protocol)
{
	if (protocol) {
		c_free(&protocol->protocol_name);
		if (protocol->src_port_array) {
			int	i;
			for (i=0; i < protocol->src_port_count; i++) {
				char *port = g_ptr_array_index(protocol->src_port_array, i);
				c_free(&port);
			}
			g_ptr_array_free(protocol->src_port_array, TRUE);
			protocol->src_port_count = 0;
		}

		if (protocol->dst_port_array) {
			int	i;
			for (i=0; i < protocol->dst_port_count; i++) {
				char *port = g_ptr_array_index(protocol->dst_port_array, i);
				c_free(&port);
			}
			g_ptr_array_free(protocol->dst_port_array, TRUE);
			protocol->dst_port_count = 0;
		}
		memset(protocol, 0, sizeof(GracRuleProtocol));
	}
}

void grac_rule_protocol_free(GracRuleProtocol **pRule)
{
	if (pRule) {
		GracRuleProtocol* rule = *pRule;
		if (rule) {
			grac_rule_protocol_clear(rule);
			free(rule);
		}
		*pRule = NULL;
	}
}

GracRuleNetwork* grac_rule_network_alloc()
{
	GracRuleNetwork* rule = malloc(sizeof(GracRuleNetwork));
	if (rule) {
		c_memset(rule, 0, sizeof(GracRuleNetwork));
	}
	return rule;
}

void grac_rule_network_free(GracRuleNetwork **pRule)
{
	if (pRule) {
		GracRuleNetwork* rule = *pRule;
		if (rule) {
			grac_rule_network_clear(rule);
			free(rule);
		}
		*pRule = NULL;
	}
}

void grac_rule_network_clear(GracRuleNetwork *rule)
{
	if (rule) {
		c_free(&rule->addr.full_addr);
		c_free(&rule->addr.part_addr);

		c_free(&rule->mac_addr_str);

		if (rule->protocols_array) {
			int	pidx;
			for (pidx=0; pidx < rule->protocols_count; pidx++) {
				GracRuleProtocol* protocol = g_ptr_array_index(rule->protocols_array, pidx);
				grac_rule_protocol_free(&protocol);
			}
			g_ptr_array_free(rule->protocols_array, TRUE);
			rule->protocols_count = 0;
		}

		c_memset(rule, 0, sizeof(GracRuleNetwork));
	}
}

gboolean grac_rule_network_set_allow(GracRuleNetwork *rule, gboolean allow)
{
	gboolean done = FALSE;

	if (rule) {
		rule->perm.allow = allow;
		rule->perm.set = TRUE;
		done = TRUE;
	}

	return done;
}

int	grac_rule_network_get_allow(GracRuleNetwork *rule, gboolean *allow)
{
	int res = -1;

	if (rule && allow) {
		if (rule->perm.set) {
			*allow = rule->perm.allow;
			res = 1;
		} else {
			*allow = FALSE;
			res = 0;
		}
	}

	return res;
}

// address : ipv4 or ipv6, can be with mask
gboolean grac_rule_network_set_address(GracRuleNetwork *rule, char *address)
{
	gboolean done = FALSE;

	if (rule && address) {

		gchar *new_full_addr = c_strdup(address, 65535);
		gchar *new_part_addr = c_strdup(address, 65535);
		if (new_full_addr == NULL || new_part_addr == NULL) {
			grac_log_error("grac_rule_network.c (%d) : out of memory to add address", __LINE__);

			c_free(&new_full_addr);
			c_free(&new_part_addr);
			return FALSE;
		}

		c_free(&rule->addr.full_addr);
		c_free(&rule->addr.part_addr);

		rule->addr.full_addr = new_full_addr;
		rule->addr.part_addr = new_part_addr;
		rule->addr.part_mask = 0;
		rule->addr.kind = 0;

		char *ptr = c_strchr(new_part_addr, '/', 65535);
		if (ptr != NULL) {
			int	len, num;
			char *ptr_mask = ptr + 1;
			len = c_get_number(ptr_mask, 65535, &num);
			if (len > 0 && ptr_mask[len] == 0) {
				rule->addr.part_mask = num;
				*ptr = 0;		// delete mask
  			}
		}

		// ipv6, ipv4
		struct addrinfo hints;
		struct addrinfo *info_list;

		c_memset(&hints, 0, sizeof(hints));
		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		int err = getaddrinfo(rule->addr.part_addr, NULL, NULL, &info_list);
		if (err != 0) {
			grac_log_warning("Can't get address info : %s", rule->addr.part_addr);
			rule->addr.kind = 0;
			done = TRUE;
		} else {
			struct addrinfo *iter;
			// struct sockaddr_in*  sin;
			// struct sockaddr_in6* sin6;
			// char	 ip_addr[256];
			//  for (iter=info_list; iter!=NULL; iter=iter->ai_next)
			iter = info_list;
			if (iter->ai_family == AF_INET) {
				rule->addr.kind = 4;
				// sin = (void*)iter->ai_addr;
				// inet_ntop(iter->ai_family, &sin->sin_addr, ip_addr, sizeof(ip_addr));
				// grac_log_debug("IPv4 - %s -> %s", net_addr->addr_str, ip_addr);
			} else if (iter->ai_family == AF_INET6) {
				rule->addr.kind = 6;
				// sin6 = (void*)iter->ai_addr;
				// inet_ntop(iter->ai_family, &sin6->sin6_addr, ip_addr, sizeof(ip_addr));
				// grac_log_debug("IPv6 - %s -> %s", net_addr->addr_str, ip_addr);
			} else {
				rule->addr.kind = 0;
			}
			done = TRUE;
		}
	}

	return done;
}

int			 grac_rule_network_get_address(GracRuleNetwork *rule, char **address)
{
	int res = -1;

	if (rule && address) {
		if (rule->addr.kind != 0) {
			res = 1;
		} else {
			res = 0;
		}
		if (rule->addr.full_addr == NULL)
			*address = "";
		else
			*address = rule->addr.full_addr;
	}

	return res;
}

int			 grac_rule_network_get_address_ex(GracRuleNetwork *rule, char **addr, int *mask, gboolean *ipv6)
{
	int res = -1;

	if (rule && addr && mask && ipv6) {
		if (rule->addr.kind == 6) {
			res = 1;
			*ipv6 = TRUE;
		} else if (rule->addr.kind == 4) {
			res = 1;
			*ipv6 = FALSE;
		} else {
			res = 0;
			*ipv6 = FALSE;
		}
		if (rule->addr.part_addr == NULL)
			*addr = "";
		else
			*addr = rule->addr.part_addr;
		*mask = rule->addr.part_mask;
	}

	return res;
}

gboolean grac_rule_network_set_direction(GracRuleNetwork *rule, grac_network_dir_t direction)
{
	gboolean done = FALSE;

	if (rule) {
		rule->dir.dir = direction;
		rule->dir.set = TRUE;
		done = TRUE;
	}

	return done;
}

int	 grac_rule_network_get_direction(GracRuleNetwork *rule, grac_network_dir_t* direction)
{
	int res = -1;

	if (rule && direction) {
		if (rule->dir.set) {
			*direction = rule->dir.dir;
			res = 1;
		} else {
			*direction = GRAC_NETWORK_DIR_ALL;
			res = 0;
		}
	}

	return res;
}

gboolean grac_rule_network_set_mac(GracRuleNetwork *rule, char *mac)
{
	gboolean done = FALSE;

	if (rule && mac) {
		gchar *add_mac = c_strdup(mac, 256);
		if (add_mac == NULL) {
			grac_log_error("grac_rule_network.c : can't alloc to set mac address");
		} else {
			c_free(&rule->mac_addr_str);
			rule->mac_addr_str = mac;
			done = TRUE;
		}
	}

	return done;
}

int	 grac_rule_network_get_mac(GracRuleNetwork *rule, char **mac)
{
	int res = -1;

	if (rule) {
		if (c_strlen(rule->mac_addr_str, 256) > 0) {
			*mac = rule->mac_addr_str;
			res = 1;
		} else {
			*mac = NULL;
			res = 0;
		}
	}

	return res;
}

gboolean grac_rule_network_add_protocol(GracRuleNetwork *rule, char *protocol)
{
	gboolean done = FALSE;

	if (rule && protocol) {

		if (rule->protocols_array == NULL) {
			rule->protocols_array = g_ptr_array_new();
			if (rule->protocols_array == NULL) {
				grac_log_error("grac_rule_network.c : can't alloc protocol array of rule");
				return FALSE;
			}
		}

		GracRuleProtocol*	add_protocol = grac_rule_protocol_alloc();
		if (add_protocol == NULL) {
			grac_log_error("grac_rule_network.c : can't alloc to add protocol");
		} else {
			add_protocol->protocol_name = c_strdup(protocol, 256);
			if (add_protocol->protocol_name == NULL) {
				grac_log_error("grac_rule_network.c : can't alloc to add protocol");
				grac_rule_protocol_free(&add_protocol);
			} else {
				g_ptr_array_add(rule->protocols_array, add_protocol);
				rule->protocols_count++;
				done = TRUE;
			}
		}
	}

	return done;
}

int			 grac_rule_network_protocol_count(GracRuleNetwork *rule)
{
	int	count = 0;
	if (rule)
		count = rule->protocols_count;
	return count;
}

int			 grac_rule_network_get_protocol(GracRuleNetwork *rule, int idx, char **protocol_name)
{
	int res = -1;

	if (rule && protocol_name) {
		if (idx >= 0 && idx < rule->protocols_count) {
			GracRuleProtocol *proto = g_ptr_array_index(rule->protocols_array, idx);
			if (proto) {
				if (c_strlen(proto->protocol_name, 256) > 0) {
					*protocol_name = proto->protocol_name;
					res = 1;
				} else {
					*protocol_name = NULL;
					res = 0;
				}
			}
		}
	}

	return res;
}

int grac_rule_network_find_protocol(GracRuleNetwork *rule, char *protocol_name)
{
	int found = -1;

	if (rule && protocol_name) {
		int idx;
		for (idx=0; idx < rule->protocols_count; idx++) {
			GracRuleProtocol *proto = g_ptr_array_index(rule->protocols_array, idx);
			if (proto) {
				if (c_strmatch(proto->protocol_name, protocol_name)) {
					found = idx;
					break;
				}
			}
		}
	}

	return found;
}


gboolean grac_rule_network_add_src_port(GracRuleNetwork *rule, int protocol_idx, char *port)
{
	gboolean done = FALSE;

	if (rule && port) {

		if (protocol_idx < 0 || protocol_idx >= rule->protocols_count) {
			grac_log_error("grac_rule_network.c : invalid index to protocol");
			return FALSE;
		}

		GracRuleProtocol *protocol = g_ptr_array_index(rule->protocols_array, protocol_idx);
		if (protocol == NULL) {
			grac_log_error("grac_rule_network.c : invalid data (null) is saved for protocol");
			return FALSE;
		}

		if (protocol->src_port_array == NULL) {
			protocol->src_port_array = g_ptr_array_new();
			if (protocol->src_port_array == NULL) {
				grac_log_error("grac_rule_network.c : can't alloc port array of rule");
				return FALSE;
			}
		}

		gchar *add_port = c_strdup(port, 256);
		if (add_port == NULL) {
			grac_log_error("grac_rule_network.c : can't alloc to add port");
		} else {
			g_ptr_array_add(protocol->src_port_array, add_port);
			protocol->src_port_count++;
			done = TRUE;
		}
	}

	return done;
}

int	grac_rule_network_src_port_count(GracRuleNetwork *rule, int protocol_idx)
{
	int	count = 0;

	if (rule) {
		if (protocol_idx < 0 || protocol_idx >= rule->protocols_count) {
			grac_log_error("grac_rule_network.c : invalid index to protocol");
			return 0;
		}

		GracRuleProtocol *protocol = g_ptr_array_index(rule->protocols_array, protocol_idx);
		if (protocol == NULL) {
			grac_log_error("grac_rule_network.c : invalid data (null) is saved for protocol");
			return 0;
		}

		count = protocol->src_port_count;
	}

	return count;
}

int grac_rule_network_get_src_port(GracRuleNetwork *rule, int protocol_idx,  int idx, char **port)
{
	int res = -1;

	if (rule && port) {
		if (protocol_idx < 0 || protocol_idx >= rule->protocols_count) {
			grac_log_error("grac_rule_network.c : invalid index to protocol");
			return res;
		}

		GracRuleProtocol *protocol = g_ptr_array_index(rule->protocols_array, protocol_idx);
		if (protocol == NULL) {
			grac_log_error("grac_rule_network.c : invalid data (null) is saved for protocol");
			return res;
		}

		if (idx >= 0 && idx < protocol->src_port_count) {
			char *set_port = g_ptr_array_index(protocol->src_port_array, idx);
			if (c_strlen(set_port, 256) > 0) {
				*port = set_port;
				res = 1;
			} else {
				*port = NULL;
				res = 0;
			}
		}
	}

	return res;
}

gboolean grac_rule_network_add_dst_port(GracRuleNetwork *rule, int protocol_idx, char *port)
{
	gboolean done = FALSE;

	if (rule && port) {
		if (protocol_idx < 0 || protocol_idx >= rule->protocols_count) {
			grac_log_error("grac_rule_network.c : invalid index to protocol");
			return FALSE;
		}

		GracRuleProtocol *protocol = g_ptr_array_index(rule->protocols_array, protocol_idx);
		if (protocol == NULL) {
			grac_log_error("grac_rule_network.c : invalid data (null) is saved for protocol");
			return FALSE;
		}

		if (protocol->dst_port_array == NULL) {
			protocol->dst_port_array = g_ptr_array_new();
			if (protocol->dst_port_array == NULL) {
				grac_log_error("grac_rule_network.c : can't alloc port array of rule");
				return FALSE;
			}
		}

		gchar *add_port = c_strdup(port, 256);
		if (add_port == NULL) {
			grac_log_error("grac_rule_network.c : can't alloc to add port");
		} else {
			g_ptr_array_add(protocol->dst_port_array, add_port);
			protocol->dst_port_count++;
			done = TRUE;
		}
	}

	return done;
}

int	grac_rule_network_dst_port_count(GracRuleNetwork *rule, int protocol_idx)
{
	int	count = 0;

	if (rule) {
		if (protocol_idx < 0 || protocol_idx >= rule->protocols_count) {
			grac_log_error("grac_rule_network.c : invalid index to protocol");
			return 0;
		}

		GracRuleProtocol *protocol = g_ptr_array_index(rule->protocols_array, protocol_idx);
		if (protocol == NULL) {
			grac_log_error("grac_rule_network.c : invalid data (null) is saved for protocol");
			return 0;
		}

		count = protocol->dst_port_count;
	}

	return count;
}

int grac_rule_network_get_dst_port(GracRuleNetwork *rule, int protocol_idx, int idx, char **port)
{
	int res = -1;

	if (rule && port) {
		if (protocol_idx < 0 || protocol_idx >= rule->protocols_count) {
			grac_log_error("grac_rule_network.c : invalid index to protocol");
			return res;
		}

		GracRuleProtocol *protocol = g_ptr_array_index(rule->protocols_array, protocol_idx);
		if (protocol == NULL) {
			grac_log_error("grac_rule_network.c : invalid data (null) is saved for protocol");
			return res;
		}

		if (idx >= 0 && idx < protocol->dst_port_count) {
			char *set_port = g_ptr_array_index(protocol->dst_port_array, idx);
			if (c_strlen(set_port, 256) > 0) {
				*port = set_port;
				res = 1;
			} else {
				*port = NULL;
				res = 0;
			}
		}
	}

	return res;
}

