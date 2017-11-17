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
 * sys_ipt.c
 *
 *  Created on: 2017. 7. 13.
 *      Author: gooroom@gooroom.kr
 */

/**
  @file 	 	sys_ipt.c
  @brief		리눅스의 iptables에 의한 네트워크 사용 허가 설정 관련 함수
  @details	시스템 혹은 특정 라이브러리 의존적인 부분을 분리하는 목적의 래퍼 함수들이다.  \n
  				현재는 "iptables"  명령을 사용하였다.	\n
  				헤더파일 :  	sys_ipt.h	\n
  				라이브러리:	libgrac-common.so
  	@notice libiptc를 이용한 cmd 방식의 의존성 제거는 보류	\n
  	 	 	 	 	 	 (공식사이트에서 libiptc사용을 하지말라고 권고함)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "sys_ipt.h"
#include "sys_etc.h"
#include "cutility.h"
#include "grac_log.h"

#define MAX_PORT_COUNT 100

struct _sys_ipt_rule {
	int	target;				// ACCEPT, DROP
	int	chain_set;		// INPUT, OUTPUT, INOUT

	// protocol
	int		protocol_type;	  // 0:not set(=all)  1:single
	int		protocol_number;
	char	protocol_name[32];

	// don't use
	// old method --> this may be deleted
	// port [0]:from  [1]:to
	int		port_type[2];				// 0:not set 1:normal(well-known)  2:not well-known
	int		port_number[2];
	char	port_name[2][32];

	// address
	int	addr_data_type;		// 0:no data 1:addr_data 2: set_name (ipset)
	int	addr_version;			// 0:not set 1:data IPv4 2:IPv6
	union {
		guint8	ip_v4[4];		// 127.0.10.1  --> [0]127 [1]:0 [2]:10 [3]:1
		guint16	ip_v6[8];
		gchar		ipset_name[256];	// set name of "ipset"
	} addr_data;
	char	addr_str[256];		    // dot style address

	// mask
	unsigned mask;				// 0 is no mask

	// MAC
	int		mac_type;			// 0:no data  1: mac_addr is set
	unsigned char	mac_addr[6];

	// new style port
	int	src_port_count;
	char *src_port_str[MAX_PORT_COUNT];
	int	dst_port_count;
	char *dst_port_str[MAX_PORT_COUNT];
};


//------------------------------------------------------------------------------------
// rule handling
//------------------------------------------------------------------------------------
sys_ipt_rule *sys_ipt_rule_alloc()
{
	sys_ipt_rule *rule = malloc(sizeof(sys_ipt_rule));
	if (rule) {
		sys_ipt_rule_init(rule);
	}

	return rule;
}

void sys_ipt_rule_free(sys_ipt_rule **prule)
{
	if (prule) {
		sys_ipt_rule *rule;
		rule = *prule;
		if (rule) {
			sys_ipt_rule_clear_src_port(rule);
			sys_ipt_rule_clear_dst_port(rule);
			free(rule);
		}
		*prule = NULL;
	}
}

void	sys_ipt_rule_init(sys_ipt_rule *rule)
{
	if (rule) {
		c_memset(rule, 0, sizeof(sys_ipt_rule));
	}
}

gboolean	sys_ipt_rule_set_target(sys_ipt_rule *rule, int target)
{
	gboolean done = FALSE;

	if (rule) {
		if (target == SYS_IPT_TARGET_ACCEPT || target == SYS_IPT_TARGET_DROP) {
			rule->target = target;
			done = TRUE;
		}
	}

	return done;
}

gboolean	sys_ipt_rule_set_chain(sys_ipt_rule *rule, int chain)
{
	gboolean done = FALSE;

	if (rule) {
		rule->chain_set = chain;
		done = TRUE;
	}

	return done;
}

gboolean	sys_ipt_rule_set_protocol(sys_ipt_rule *rule, int protocol)
{
	gboolean done = FALSE;

	if (rule) {
		if (protocol == -1) {
			rule->protocol_type = 0;
			rule->protocol_number = -1;
			rule->protocol_name[0] = 0;
			done = TRUE;
		} else {
			struct protoent *ent = getprotobynumber(protocol);
			if (ent) {
				rule->protocol_type = 1;
				rule->protocol_number = protocol;
				c_strcpy(rule->protocol_name, ent->p_name, sizeof(rule->protocol_name));
				done = TRUE;
			} else {
				grac_log_error("sys_ipt.c : unknown protocol name");
			}
		}
	}

	return done;
}

gboolean	sys_ipt_rule_set_protocol_name(sys_ipt_rule *rule, char *proto_name)
{
	gboolean done = FALSE;

	if (rule) {
		if (c_strcmp(proto_name, "all", 4, -2) == 0 || c_strcmp(proto_name, "ALL", 4, -2) == 0) {
			rule->protocol_type = 0;
			rule->protocol_number = -1;
			rule->protocol_name[0] = 0;
			done = TRUE;
		} else {
			struct protoent *ent = getprotobyname(proto_name);
			if (ent) {
				rule->protocol_type = 1;
				rule->protocol_number = ent->p_proto;
				c_strcpy(rule->protocol_name, ent->p_name, sizeof(rule->protocol_name));
				done = TRUE;
			} else {
				grac_log_error("sys_ipt.c : unknown protocol number");
			}
		}
	}

	return done;
}

static gboolean	_sys_ipt_rule_set_port_number(sys_ipt_rule *rule, int idx, int port)
{
	gboolean done = FALSE;

	if (idx < 0 || idx > 1)
		return FALSE;

	if (rule) {
		if (rule->protocol_type != 1) {
			grac_log_error("sys_ipt.c : meaningless port : not defined protocol");
			return FALSE;
		}
		if (rule->protocol_number != SYS_IPT_PROTOCOL_TCP &&
				rule->protocol_number != SYS_IPT_PROTOCOL_UDP)
        {
			grac_log_error("sys_ipt.c : meaningless port : protocol dose not support ports");
			return FALSE;
		}

		if (port == -1)
			port = 65535;

		struct servent *ent;
		ent = getservbyport(port, rule->protocol_name);
		if (ent) {
			rule->port_type[idx] = 1;
			rule->port_number[idx] = ent->s_port;
			c_strcpy(rule->port_name[idx], ent->s_name, sizeof(rule->port_name[idx]));
		} else {  // no error
			rule->port_type[idx] = 2;
			rule->port_number[idx] = port;
			rule->port_name[idx][0] = 0;
		}
		done = TRUE;
	}

	return done;
}

static gboolean	_sys_ipt_rule_set_port_name(sys_ipt_rule *rule, int idx, char *name)
{
	gboolean done = FALSE;

	if (idx < 0 || idx > 1)
		return FALSE;

	if (c_strlen(name, 128) <= 0)
		return FALSE;

	if (rule) {
		if (rule->protocol_type != 1) {
			grac_log_error("sys_ipt.c : meaningless port : not defined protocol");
			return FALSE;
		}
		if (rule->protocol_number != SYS_IPT_PROTOCOL_TCP &&
				rule->protocol_number != SYS_IPT_PROTOCOL_UDP)
        {
			grac_log_error("sys_ipt.c : meaningless port : protocol dose not support ports");
			return FALSE;
		}

		struct servent *ent;
		ent = getservbyname(name, rule->protocol_name);
		if (ent) {
			rule->port_type[idx] = 1;
			rule->port_number[idx] = ent->s_port;
			c_strcpy(rule->port_name[idx], ent->s_name, sizeof(rule->port_name[idx]));
			done = TRUE;
		} else {  // error
			rule->port_type[idx] = 0;
			rule->port_name[idx][0] = 0;
			grac_log_error("sys_ipt.c : unknown port name");
		}
	}

	return done;
}

gboolean	sys_ipt_rule_set_port(sys_ipt_rule *rule, int port)
{
	gboolean done = FALSE;

	if (rule) {
		done = _sys_ipt_rule_set_port_number(rule, 0, port);
		rule->port_type[1] = 0;
		rule->port_name[1][0] = 0;
	}

	return done;
}

gboolean	sys_ipt_rule_set_port2(sys_ipt_rule *rule, int from_port, int to_port)
{
	gboolean done = FALSE;

	if (rule) {
		done  = _sys_ipt_rule_set_port_number(rule, 0, from_port);
		done &= _sys_ipt_rule_set_port_number(rule, 1, to_port);
	}

	return done;
}

gboolean	sys_ipt_rule_set_port_name(sys_ipt_rule *rule, gchar *name)
{
	gboolean done = FALSE;

	if (rule) {
		done = _sys_ipt_rule_set_port_name(rule, 0, name);
		rule->port_type[1] = 0;
		rule->port_name[1][0] = 0;
	}

	return done;
}

gboolean	sys_ipt_rule_set_port_name2(sys_ipt_rule *rule, gchar *from_name, gchar *to_name)
{
	gboolean done = FALSE;

	if (rule) {
		done  = _sys_ipt_rule_set_port_name(rule, 0, from_name);
		done &= _sys_ipt_rule_set_port_name(rule, 1, to_name);
	}

	return done;
}

// 80,  80-90,  80:90
gboolean	sys_ipt_rule_set_port_str(sys_ipt_rule *rule, gchar *port_str)
{
	gboolean done = FALSE;

	if (rule && port_str) {
		char *ptr = c_strchr(port_str, ':', 65535);
		if (ptr == NULL)
			ptr = c_strchr(port_str, '-', 65535);
		if (ptr == NULL)
			ptr = c_strchr(port_str, '~', 65535);

		if (ptr == NULL) {
			done  = _sys_ipt_rule_set_port_name(rule, 0, port_str);
		} else {
			char ch = *ptr;
			*ptr = 0;
			done  = _sys_ipt_rule_set_port_name(rule, 0, port_str);
			done &= _sys_ipt_rule_set_port_name(rule, 1, ptr+1);
			*ptr = ch;
		}
	}

	return done;
}

void			sys_ipt_rule_clear_src_port(sys_ipt_rule *rule)
{
	if (rule) {
		int	i;
		for (i=0; i < rule->src_port_count; i++) {
			if (rule->src_port_str[i])
				free(rule->src_port_str[i]);
			rule->src_port_str[i] = NULL;
		}
		rule->src_port_count = 0;
	}
}

void			sys_ipt_rule_clear_dst_port(sys_ipt_rule *rule)
{
	if (rule) {
		int	i;
		for (i=0; i < rule->dst_port_count; i++) {
			if (rule->dst_port_str[i])
				free(rule->dst_port_str[i]);
			rule->dst_port_str[i] = NULL;
		}
		rule->dst_port_count = 0;
	}
}

// single or range
gboolean	sys_ipt_rule_add_src_port_str(sys_ipt_rule *rule, gchar *port_str)
{
	gboolean done = FALSE;

	if (rule && port_str) {
		if (rule->src_port_count < MAX_PORT_COUNT) {
			char *add_port = c_strdup(port_str, 256);
			if (add_port != NULL) {
				char *ptr = c_strchr(add_port, '-', 256);
				if (ptr)
					*ptr = ':';
				ptr = c_strchr(add_port, '~', 256);
				if (ptr)
					*ptr = ':';
				rule->src_port_str[rule->src_port_count] = add_port;
				rule->src_port_count++;
				done = TRUE;
			}
		}
	}

	return done;
}

// single or range
gboolean	sys_ipt_rule_add_dst_port_str(sys_ipt_rule *rule, gchar *port_str)
{
	gboolean done = FALSE;

	if (rule && port_str) {
		if (rule->dst_port_count < MAX_PORT_COUNT) {
			char *add_port = c_strdup(port_str, 256);
			if (add_port != NULL) {
				char *ptr = c_strchr(add_port, '-', 256);
				if (ptr)
					*ptr = ':';
				ptr = c_strchr(add_port, '~', 256);
				if (ptr)
					*ptr = ':';
				rule->dst_port_str[rule->dst_port_count] = add_port;
				rule->dst_port_count++;
				done = TRUE;
			}
		}
	}

	return done;
}


gboolean	sys_ipt_rule_set_addr(sys_ipt_rule *rule, guint8 addr[4])
{
	gboolean done = FALSE;

	if (rule) {
		rule->addr_data_type = 1;
		rule->addr_version = 1;
		c_memcpy(rule->addr_data.ip_v4, addr, sizeof(rule->addr_data.ip_v4));
		const char *res;
		res = inet_ntop(AF_INET, rule->addr_data.ip_v4, rule->addr_str, sizeof(rule->addr_str) );
		if (res == NULL) {
			grac_log_warning("sys_ipt.c : invalid V4 address data : %s", c_strerror(-1));
			rule->addr_str[0] = 0;
		}

		done = TRUE;
	}

	return done;
}

gboolean	sys_ipt_rule_set_addr6(sys_ipt_rule *rule, guint16 addr[8])
{
	gboolean done = FALSE;

	if (rule) {
		rule->addr_data_type = 1;
		rule->addr_version = 2;
		c_memcpy(rule->addr_data.ip_v6, addr, sizeof(rule->addr_data.ip_v6));
		const char *res;
		res = inet_ntop(AF_INET6, rule->addr_data.ip_v6, rule->addr_str, sizeof(rule->addr_str) );
		if (res == NULL) {
			grac_log_warning("sys_ipt.c : invalid V6 address data : %s", c_strerror(-1));
			rule->addr_str[0] = 0;
		}

		done = TRUE;
	}

	return done;
}

gboolean	sys_ipt_rule_set_addr_str(sys_ipt_rule *rule, gchar *addr_str)
{
	gboolean done = FALSE;

	if (rule) {
		struct in_addr inaddr;
		int res = inet_pton(AF_INET, addr_str, &inaddr);
		if (res == 1) {
			rule->addr_data_type = 1;
			rule->addr_version = 1;
			c_strcpy(rule->addr_str, addr_str, sizeof(rule->addr_str));
			c_memcpy(rule->addr_data.ip_v4, &inaddr, sizeof(rule->addr_data.ip_v4));
			done = TRUE;
		} else {
			grac_log_warning("sys_ipt_rule_set_addr_str() : %s", c_strerror(-1));
			rule->addr_data_type = 0;
			rule->addr_version = 1;
			c_strcpy(rule->addr_str, addr_str, sizeof(rule->addr_str));
			c_memcpy(rule->addr_data.ip_v4, 0, sizeof(rule->addr_data.ip_v4));
			done = TRUE;
		}
	}

	return done;
}

gboolean	sys_ipt_rule_set_addr6_str(sys_ipt_rule *rule, gchar *addr_str)
{
	gboolean done = FALSE;

	if (rule) {
		struct in6_addr inaddr;
		int res = inet_pton(AF_INET6, addr_str, &inaddr);
		if (res == 1) {
			rule->addr_data_type = 1;
			rule->addr_version = 2;
			c_strcpy(rule->addr_str, addr_str, sizeof(rule->addr_str));
			c_memcpy(rule->addr_data.ip_v6, &inaddr, sizeof(rule->addr_data.ip_v6));
			done = TRUE;
		} else {
			grac_log_warning("sys_ipt_rule_set_addr_str() : %s", c_strerror(-1));
			rule->addr_data_type = 0;
			rule->addr_version = 2;
			c_strcpy(rule->addr_str, addr_str, sizeof(rule->addr_str));
			c_memset(rule->addr_data.ip_v6, 0, sizeof(rule->addr_data.ip_v6));
			done = TRUE;
		}
	}

	return done;
}

gboolean	sys_ipt_rule_set_ipset(sys_ipt_rule *rule, gchar* setname)
{
	gboolean done = FALSE;

	if (c_strlen(setname, 256) <= 0)
		return FALSE;

	if (rule) {
		rule->addr_data_type = 2;
		rule->addr_version = 1;
		c_strcpy(rule->addr_data.ipset_name, setname, sizeof(rule->addr_data.ipset_name));
		rule->addr_str[0] = 0;
		done = TRUE;
	}

	return done;
}

gboolean	sys_ipt_rule_set_ipset6(sys_ipt_rule *rule, gchar* setname)
{
	gboolean done = FALSE;

	if (c_strlen(setname, 256) <= 0)
		return FALSE;

	if (rule) {
		rule->addr_data_type = 2;
		rule->addr_version = 2;
		c_strcpy(rule->addr_data.ipset_name, setname, sizeof(rule->addr_data.ipset_name));
		rule->addr_str[0] = 0;
		done = TRUE;
	}

	return done;
}

gboolean	sys_ipt_rule_set_mask(sys_ipt_rule *rule, guchar mask)
{
	gboolean done = FALSE;

	if (rule) {
		rule->mask = mask;
		done = TRUE;
	}

	return done;
}


gboolean	sys_ipt_rule_set_mac_addr(sys_ipt_rule *rule, guint8 mac[6])
{
	gboolean done = FALSE;
	if (rule) {
		rule->mac_type = 1;
		c_memcpy(rule->mac_addr, mac, sizeof(rule->mac_addr));
		done = TRUE;
	}

	return done;
}

gboolean	sys_ipt_rule_set_mac_addr_str(sys_ipt_rule *rule, gchar *mac_str)
{
	gboolean done = FALSE;
	guint8 mac_addr[6];

	if (rule && mac_str) {
		guint	v[6];
		int		n;
		char	ch;

		n = sscanf(mac_str, "%x:%x:%x:%x:%x:%x%c", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &ch);
		if (n == 6) {
			int i;
			for (i=0; i < 6; i++) {
				if (v[i] >= 0 && v[i] <= 255) {
					mac_addr[i] = v[i];
				} else {
					break;
				}
			}
			if (i == 6)
				done = sys_ipt_rule_set_mac_addr(rule, mac_addr);
			else
				grac_log_error("sys_ipt.c : invalid mac address : %s", mac_str);
		}
	}

	return done;
}


//------------------------------------------------------------------------------------
// control iptables
//------------------------------------------------------------------------------------

struct _sys_ipt {
	gboolean log_on;
	char *log_head;
	int	 log_head_size;
	char *set_cmd;
	int	 set_cmd_size;
	char *log_cmd;
	int	 log_cmd_size;

	char	*tmp_buf;
	int		tmp_buf_size;
};

sys_ipt *sys_ipt_alloc()
{
	sys_ipt *ipt = malloc(sizeof(sys_ipt));
	if (ipt) {
		c_memset(ipt, 0, sizeof(sys_ipt));
		ipt->log_head_size = 256;
		ipt->log_head = c_alloc(ipt->log_head_size);
		ipt->set_cmd_size = 2048;
		ipt->set_cmd = c_alloc(ipt->set_cmd_size);
		ipt->log_cmd_size = 2048;
		ipt->log_cmd = c_alloc(ipt->log_cmd_size);
		ipt->tmp_buf_size = 1024;
		ipt->tmp_buf = c_alloc(ipt->tmp_buf_size);

		if (ipt->log_head == NULL ||
				ipt->set_cmd == NULL ||
				ipt->log_cmd == NULL ||
				ipt->tmp_buf == NULL)
        {
			sys_ipt_free(&ipt);
		}
	}

	return ipt;
}

void sys_ipt_free(sys_ipt **pipt)
{
	sys_ipt *ipt;

	if (pipt) {
		ipt = *pipt;
		if (ipt) {
			c_free(&ipt->log_head);
			c_free(&ipt->set_cmd);
			c_free(&ipt->log_cmd);
			c_free(&ipt->tmp_buf);

			free(ipt);
		}
		*pipt = NULL;
	}
}

/**
  @brief   iptables에 등록되어 있는 모든 rule을 초기화한다.
  @return gboolean 성공여부
 */
gboolean	sys_ipt_clear_all(sys_ipt* ipt)
{
	gboolean res;
	gchar	*cmd;

	cmd = "iptables -F OUTPUT 2>&1";
	res = sys_run_cmd_no_output(cmd, "sys_ipt");

	cmd = "iptables -F INPUT 2>&1";
	res &= sys_run_cmd_no_output(cmd, "sys_ipt");

	return res;
}


static gboolean _make_and_run_cmd(sys_ipt* ipt, gboolean append, int chain, sys_ipt_rule *rule)
{
	gboolean done = FALSE;
	int	len;

	// command
	if (rule->addr_version == 2)
		c_strcpy(ipt->set_cmd, "ip6tables ", ipt->set_cmd_size);
	else
		c_strcpy(ipt->set_cmd, "iptables ", ipt->set_cmd_size);

	// location of list
	if (append == TRUE)
		c_strcat(ipt->set_cmd, "-A ", ipt->set_cmd_size);
	else
		c_strcat(ipt->set_cmd, "-I ", ipt->set_cmd_size);

	if (chain == SYS_IPT_CHAIN_B_INPUT)
		c_strcat(ipt->set_cmd, "INPUT ", ipt->set_cmd_size);
	else if (chain == SYS_IPT_CHAIN_B_OUTPUT)
		c_strcat(ipt->set_cmd, "OUTPUT ", ipt->set_cmd_size);
	else
		return FALSE;

	// address
	if (rule->addr_str[0] != 0) {   // if (rule->addr_data_type == 1)
		if (chain == SYS_IPT_CHAIN_B_INPUT)
			c_strcat(ipt->set_cmd, "-s ", ipt->set_cmd_size);
		else   // output
			c_strcat(ipt->set_cmd, "-d ", ipt->set_cmd_size);
		c_strcat(ipt->set_cmd, rule->addr_str, ipt->set_cmd_size);

		if (rule->mask) {
			if (rule->addr_version == 2)
				g_snprintf(ipt->tmp_buf, ipt->tmp_buf_size, "/%d ", rule->mask % 129);
			else
				g_snprintf(ipt->tmp_buf, ipt->tmp_buf_size, "/%d ", rule->mask % 33);
		} else {
			c_strcpy(ipt->tmp_buf, " ", ipt->tmp_buf_size);
		}

		c_strcat(ipt->set_cmd, ipt->tmp_buf, ipt->set_cmd_size);
	}

	// protocol
	if (rule->protocol_type != 0) {
		c_strcat(ipt->set_cmd, "-p ", ipt->set_cmd_size);

		if (rule->protocol_name[0] != 0) {
			c_strcat(ipt->set_cmd, rule->protocol_name, ipt->set_cmd_size);
		} else {
			g_snprintf(ipt->tmp_buf, ipt->tmp_buf_size, "%u", rule->protocol_number);
			c_strcat(ipt->set_cmd, ipt->tmp_buf, ipt->set_cmd_size);
		}
		c_strcat(ipt->set_cmd, " ", ipt->set_cmd_size);
	}

	// port
	if (rule->protocol_number == SYS_IPT_PROTOCOL_TCP ||
			rule->protocol_number == SYS_IPT_PROTOCOL_UDP)
    {
		if (rule->src_port_count > 1 || rule->dst_port_count > 1) {
			c_strcat(ipt->set_cmd, "-m multiport ", ipt->set_cmd_size);
			if (rule->src_port_count > 0) {
				int i;
				c_strcat(ipt->set_cmd, "--sports ", ipt->set_cmd_size);
				for (i=0; i < rule->src_port_count; i++) {
					if (i > 0)
						c_strcat(ipt->set_cmd, ",", ipt->set_cmd_size);
					c_strcat(ipt->set_cmd, rule->src_port_str[i], ipt->set_cmd_size);
				}
				c_strcat(ipt->set_cmd, " ", ipt->set_cmd_size);
			}
			if (rule->dst_port_count > 0) {
				int i;
				c_strcat(ipt->set_cmd, "--dports ", ipt->set_cmd_size);
				for (i=0; i < rule->dst_port_count; i++) {
					if (i > 0)
						c_strcat(ipt->set_cmd, ",", ipt->set_cmd_size);
					c_strcat(ipt->set_cmd, rule->dst_port_str[i], ipt->set_cmd_size);
				}
				c_strcat(ipt->set_cmd, " ", ipt->set_cmd_size);
			}
		} else {
			if (rule->src_port_count == 1) {
				c_strcat(ipt->set_cmd, "--sport ", ipt->set_cmd_size);
				c_strcat(ipt->set_cmd, rule->src_port_str[0], ipt->set_cmd_size);
				c_strcat(ipt->set_cmd, " ", ipt->set_cmd_size);
			}
			if (rule->dst_port_count == 1) {
				c_strcat(ipt->set_cmd, "--dport ", ipt->set_cmd_size);
				c_strcat(ipt->set_cmd, rule->dst_port_str[0], ipt->set_cmd_size);
				c_strcat(ipt->set_cmd, " ", ipt->set_cmd_size);
			}
		}
	}

	// ipset
	if (rule->addr_data_type == 2) {
		int len = c_strlen(ipt->set_cmd, ipt->set_cmd_size);
		if (chain == SYS_IPT_CHAIN_B_INPUT)
			g_snprintf(ipt->set_cmd+len, ipt->set_cmd_size-len, "-m set --match-set %s src ", rule->addr_data.ipset_name);
		else
			g_snprintf(ipt->set_cmd+len, ipt->set_cmd_size-len, "-m set %s --match-set dst ", rule->addr_data.ipset_name);
	}

	// MAC
	if (rule->mac_type == 1) {
		if (chain == SYS_IPT_CHAIN_B_INPUT) {
			int len = c_strlen(ipt->set_cmd, ipt->set_cmd_size);
			g_snprintf(ipt->set_cmd+len, ipt->set_cmd_size-len, "-m mac --mac-source %02x:%02x:%02x:%02x:%02x:%02x ",
																					rule->mac_addr[0], rule->mac_addr[1], rule->mac_addr[2],
																					rule->mac_addr[3], rule->mac_addr[4], rule->mac_addr[5] );
		}
	}

	if (ipt->log_on) {
		g_snprintf(ipt->log_cmd, ipt->log_cmd_size,
							"%s -j LOG --log-level error --log-prefix '%s'", ipt->set_cmd, ipt->log_head);
	}

	// target
	if (rule->target == SYS_IPT_TARGET_ACCEPT)
		c_strcat(ipt->set_cmd, "-j ACCEPT", ipt->set_cmd_size);
	else if (rule->target == SYS_IPT_TARGET_DROP)
		c_strcat(ipt->set_cmd, "-j DROP", ipt->set_cmd_size);
	else
		return FALSE;

	c_strcat(ipt->set_cmd, "?", ipt->set_cmd_size);

	len = c_strlen(ipt->set_cmd, ipt->set_cmd_size);
	if (len <= 0 || ipt->set_cmd[len-1] != '?') {
		grac_log_error("sys_ipt.c: Made command is invalid [%s]", ipt->set_cmd);
		return FALSE;
	}
	ipt->set_cmd[len-1] = 0;

	// actual calling the iptables command for LOG
	if (ipt->log_on && append == TRUE) {
		gboolean res = sys_run_cmd_no_output(ipt->log_cmd, "sys_ipt(log)");
		grac_log_debug("sys_ipt.c: run command [%s] : res=%d", ipt->log_cmd, (int)res);
	}

	// actual calling the iptables command.
	done = sys_run_cmd_no_output(ipt->set_cmd, "sys_ipt");
	grac_log_debug("sys_ipt.c: run command [%s] : res=%d", ipt->set_cmd, (int)done);

	if (ipt->log_on && append == FALSE) {
		gboolean res = sys_run_cmd_no_output(ipt->log_cmd, "sys_ipt(log)");
		grac_log_debug("sys_ipt.c: run command [%s] : res=%d", ipt->log_cmd, (int)res);
	}

	return done;
}

static gboolean _apply_rule(sys_ipt* ipt, gboolean append, sys_ipt_rule *rule)
{
	gboolean done = FALSE;

	if (rule) {
		done = TRUE;
		if (rule->chain_set == 0) {
			done &= _make_and_run_cmd(ipt, append, SYS_IPT_CHAIN_B_INPUT,  rule);
			done &= _make_and_run_cmd(ipt, append, SYS_IPT_CHAIN_B_OUTPUT, rule);
		} else {
			if (rule->chain_set & SYS_IPT_CHAIN_B_INPUT)
				done &= _make_and_run_cmd(ipt, append, SYS_IPT_CHAIN_B_INPUT, rule);
			if (rule->chain_set & SYS_IPT_CHAIN_B_OUTPUT)
				done &= _make_and_run_cmd(ipt, append, SYS_IPT_CHAIN_B_OUTPUT, rule);
		}
	}

	return done;
}

gboolean	sys_ipt_insert_rule(sys_ipt* ipt, sys_ipt_rule *rule)
{
	return _apply_rule (ipt, FALSE, rule);
}

gboolean	sys_ipt_append_rule(sys_ipt *ipt, sys_ipt_rule *rule)
{
	return _apply_rule (ipt, TRUE, rule);
}

static gboolean	_sys_ipt_add_all_target_rule(sys_ipt* ipt, gboolean append, int target)
{
	gboolean done = TRUE;
	sys_ipt_rule rule;

	gboolean save = ipt->log_on;
	ipt->log_on = FALSE;

	sys_ipt_rule_init(&rule);
	rule.chain_set = SYS_IPT_CHAIN_B_INOUT;
	sys_ipt_rule_set_target(&rule, target);

	done &= _apply_rule(ipt, append, &rule);

	rule.addr_version = 2;
	done &= _apply_rule(ipt, append, &rule);

	ipt->log_on = save;

	return done;
}


gboolean	sys_ipt_insert_all_drop_rule(sys_ipt* ipt)
{
	return _sys_ipt_add_all_target_rule (ipt, FALSE, SYS_IPT_TARGET_DROP);
}

gboolean	sys_ipt_append_all_drop_rule(sys_ipt* ipt)
{
	return _sys_ipt_add_all_target_rule (ipt, TRUE, SYS_IPT_TARGET_DROP);
}

gboolean	sys_ipt_insert_all_accept_rule(sys_ipt* ipt)
{
	return _sys_ipt_add_all_target_rule (ipt, FALSE, SYS_IPT_TARGET_ACCEPT);
}

gboolean	sys_ipt_append_all_accept_rule(sys_ipt* ipt)
{
	return _sys_ipt_add_all_target_rule (ipt, TRUE, SYS_IPT_TARGET_ACCEPT);
}

gboolean	sys_ipt_set_log(sys_ipt* ipt, gboolean log_on, char *header)
{
	gboolean done = FALSE;

	if (c_strlen(header, 256) > 0) {
		ipt->log_on = log_on;

		c_memset(ipt->log_head, 0, ipt->log_head_size);
		c_strcpy(ipt->log_head, header, ipt->log_head_size);

		done = TRUE;
	}

	return done;
}

static gboolean	_sys_ipt_set_policy_sub(sys_ipt* ipt, char *cmd, char *chain, char *target)
{
	gboolean done;

	g_snprintf(ipt->set_cmd, ipt->set_cmd_size, "%s -P %s %s", cmd, chain, target);

	done = sys_run_cmd_no_output(ipt->set_cmd, "sys_ipt");
	grac_log_debug("sys_ipt.c: run command [%s] : res=%d", ipt->set_cmd, (int)done);

	return done;
}


gboolean	sys_ipt_set_policy(sys_ipt* ipt, gboolean allow)
{
	gboolean done = TRUE;
	char 	*target;

	if (allow)
		target = "ACCEPT";
	else
		target = "DROP";

	done &= _sys_ipt_set_policy_sub(ipt, "iptables", "INPUT", target);
	done &= _sys_ipt_set_policy_sub(ipt, "iptables", "OUTPUT", target);

	done &= _sys_ipt_set_policy_sub(ipt, "ip6tables", "INPUT", target);
	done &= _sys_ipt_set_policy_sub(ipt, "ip6tables", "OUTPUT", target);

	return done;
}
