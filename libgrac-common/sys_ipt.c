/*
 * sys_ipt.c
 *
 *  Created on: 2017. 7. 13.
 *      Author: user
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

#include "sys_ipt.h"
#include "sys_etc.h"
#include "cutility.h"
#include "grm_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

struct _sys_ipt_rule {
	int	target;				// ACCEPT, DROP
	int	chain_set;		// INPUT, OUTPUT, INOUT

	// protocol
	int		protocol_type;	  // 0:not set(=all)  1:single
	int		protocol_number;
	char	protocol_name[32];

	// port [0]:from  [1]:to
	int		port_type  [2];				// 0:not set 1:normal(well-known)  2:not well-known
	int		port_number[2];
	char	port_name  [2][32];

	//address
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
	sys_ipt_rule *rule;

	if (prule) {
		rule = *prule;
		if (rule)
			free(rule);
		*prule = NULL;
	}
}

void	sys_ipt_rule_init(sys_ipt_rule *rule)
{
	if (rule) {
		c_memset(rule, 0, sizeof(sys_ipt_rule));
	}
}

gboolean	sys_ipt_rule_set_target  (sys_ipt_rule *rule, int target)
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

gboolean	sys_ipt_rule_set_chain   (sys_ipt_rule *rule, int chain)
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
		}
		else {
			struct protoent *ent = getprotobynumber(protocol);
			if (ent) {
				rule->protocol_type = 1;
				rule->protocol_number = protocol;
				c_strcpy(rule->protocol_name, ent->p_name, sizeof(rule->protocol_name));
				done = TRUE;
			}
			else {
				grm_log_error("sys_ipt.c : unknown protocol name");
			}
		}
	}

	return done;
}

gboolean	sys_ipt_rule_set_protocol_name(sys_ipt_rule *rule, char *proto_name)
{
	gboolean done = FALSE;

	if (rule) {
		if (c_strcmp(proto_name, "all", 4, -2) == 0 || c_strcmp(proto_name, "ALL", 4, -2) == 0 ) {
			rule->protocol_type = 0;
			rule->protocol_number = -1;
			rule->protocol_name[0] = 0;
			done = TRUE;
		}
		else {
			struct protoent *ent = getprotobyname(proto_name);
			if (ent) {
				rule->protocol_type = 1;
				rule->protocol_number = ent->p_proto;
				c_strcpy(rule->protocol_name, ent->p_name, sizeof(rule->protocol_name));
				done = TRUE;
			}
			else {
				grm_log_error("sys_ipt.c : unknown protocol number");
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
			grm_log_error("sys_ipt.c : meaningless port : not defined protocol");
			return FALSE;
		}
		if (rule->protocol_number != SYS_IPT_PROTOCOL_TCP &&
				rule->protocol_number != SYS_IPT_PROTOCOL_UDP)
		{
			grm_log_error("sys_ipt.c : meaningless port : protocol dose not support ports");
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
		}
		else {  // no error
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
			grm_log_error("sys_ipt.c : meaningless port : not defined protocol");
			return FALSE;
		}
		if (rule->protocol_number != SYS_IPT_PROTOCOL_TCP &&
				rule->protocol_number != SYS_IPT_PROTOCOL_UDP)
		{
			grm_log_error("sys_ipt.c : meaningless port : protocol dose not support ports");
			return FALSE;
		}

		struct servent *ent;
		ent = getservbyname(name, rule->protocol_name);
		if (ent) {
			rule->port_type[idx] = 1;
			rule->port_number[idx] = ent->s_port;
			c_strcpy(rule->port_name[idx], ent->s_name, sizeof(rule->port_name[idx]));
			done = TRUE;
		}
		else {  // error
			rule->port_type[idx] = 0;
			rule->port_name[idx][0] = 0;
			grm_log_error("sys_ipt.c : unknown port name");
		}
	}

	return done;

}

gboolean	sys_ipt_rule_set_port (sys_ipt_rule *rule, int port)
{
	gboolean done = FALSE;

	if (rule) {
		done = _sys_ipt_rule_set_port_number (rule, 0, port);
		rule->port_type[1] = 0;
		rule->port_name[1][0] = 0;
	}

	return done;
}

gboolean	sys_ipt_rule_set_port2(sys_ipt_rule *rule, int from_port, int to_port)
{
	gboolean done = FALSE;

	if (rule) {
		done  = _sys_ipt_rule_set_port_number (rule, 0, from_port);
		done &= _sys_ipt_rule_set_port_number (rule, 1, to_port);
	}

	return done;
}

gboolean	sys_ipt_rule_set_port_name (sys_ipt_rule *rule, gchar *name)
{
	gboolean done = FALSE;

	if (rule) {
		done = _sys_ipt_rule_set_port_name (rule, 0, name);
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
		}
		else {
			char ch = *ptr;
			*ptr = 0;
			done  = _sys_ipt_rule_set_port_name(rule, 0, port_str);
			done &= _sys_ipt_rule_set_port_name(rule, 1, ptr+1);
			*ptr = ch;
		}
	}

	return done;

}

gboolean	sys_ipt_rule_set_addr (sys_ipt_rule *rule, guint8 addr[4])
{
	gboolean done = FALSE;

	if (rule) {
		rule->addr_data_type = 1;
		rule->addr_version = 1;
		c_memcpy(rule->addr_data.ip_v4, addr, sizeof(rule->addr_data.ip_v4));
		const char *res;
		res = inet_ntop(AF_INET, rule->addr_data.ip_v4, rule->addr_str, sizeof(rule->addr_str) );
		if (res == NULL) {
			grm_log_warning("sys_ipt.c : invalid V4 address data : %s", strerror(errno));
			rule->addr_str[0] = 0;
		}

		done = TRUE;
	}

	return done;
}

gboolean	sys_ipt_rule_set_addr6 (sys_ipt_rule *rule, guint16 addr[8])
{
	gboolean done = FALSE;

	if (rule) {
		rule->addr_data_type = 1;
		rule->addr_version = 2;
		c_memcpy(rule->addr_data.ip_v6, addr, sizeof(rule->addr_data.ip_v6));
		const char *res;
		res = inet_ntop(AF_INET6, rule->addr_data.ip_v6, rule->addr_str, sizeof(rule->addr_str) );
		if (res == NULL) {
			grm_log_warning("sys_ipt.c : invalid V6 address data : %s", strerror(errno));
			rule->addr_str[0] = 0;
		}

		done = TRUE;
	}

	return done;
}

gboolean	sys_ipt_rule_set_addr_str (sys_ipt_rule *rule, gchar *addr_str)
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
		}
		else {
			grm_log_warning("sys_ipt_rule_set_addr_str() : %s", strerror(errno));
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
		}
		else {
			grm_log_warning("sys_ipt_rule_set_addr_str() : %s", strerror(errno));
			rule->addr_data_type = 0;
			rule->addr_version = 2;
			c_strcpy(rule->addr_str, addr_str, sizeof(rule->addr_str));
			c_memset(rule->addr_data.ip_v6, 0, sizeof(rule->addr_data.ip_v6));
			done = TRUE;
		}
	}

	return done;
}

gboolean	sys_ipt_rule_set_ipset   (sys_ipt_rule *rule, gchar* setname)
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

gboolean	sys_ipt_rule_set_ipset6   (sys_ipt_rule *rule, gchar* setname)
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
		int		n, i;
		char	ch;

		n = sscanf(mac_str, "%x:%x:%x:%x:%x:%x%c", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &ch);
		if (n == 6) {
			for (i=0; i<6; i++) {
				if (v[i] >= 0 && v[i] <= 255) {
					mac_addr[i] = v[i];
				}
				else {
					break;
				}
			}
			if (i == 6)
				done = sys_ipt_rule_set_mac_addr(rule, mac_addr);
			else
				grm_log_error("sys_ipt.c : invalid mac address : %s", mac_str);
		}
	}

	return done;
}


//------------------------------------------------------------------------------------
// control iptables
//------------------------------------------------------------------------------------

/**
  @brief   iptables에 등록되어 있는 모든 rule을 초기화한다.
  @return gboolean 성공여부
 */
gboolean	sys_ipt_clear_all()
{
	gboolean res;
	gchar	cmd[128];

	sprintf(cmd, "iptables -F OUTPUT 2>&1");
	res = sys_run_cmd_no_output(cmd, "sys_ipt");

	sprintf(cmd, "iptables -F INPUT 2>&1");
	res &= sys_run_cmd_no_output(cmd, "sys_ipt");

	return res;
}


static gboolean _make_and_run_cmd (gboolean append, int chain, sys_ipt_rule *rule)
{
	gboolean done = FALSE;
	char cmd[1024];
	int	 cmd_len = 0;

	// command
	if (rule->addr_version == 2)
		c_strcpy(cmd, "ip6tables ", sizeof(cmd));
	else
		c_strcpy(cmd, "iptables ", sizeof(cmd));

	// location of list
	if (append == TRUE)
		c_strcat(cmd, "-A ", sizeof(cmd));
	else
		c_strcat(cmd, "-I ", sizeof(cmd));

	if (chain == SYS_IPT_CHAIN_B_INPUT)
		c_strcat(cmd, "INPUT ", sizeof(cmd));
	else if (chain == SYS_IPT_CHAIN_B_OUTPUT)
		c_strcat(cmd, "OUTPUT ", sizeof(cmd));
	else
		return FALSE;

	// address
	if (rule->addr_str[0] != 0) {   // if (rule->addr_data_type == 1)
		if (chain == SYS_IPT_CHAIN_B_INPUT)
			c_strcat(cmd, "-s ", sizeof(cmd));
		else   // output
			c_strcat(cmd, "-d ", sizeof(cmd));
		c_strcat(cmd, rule->addr_str, sizeof(cmd));

		char mask_str[12];
		if (rule->mask) {
			if (rule->addr_version == 2)
				snprintf(mask_str, sizeof(mask_str), "/%d ", rule->mask % 129);
			else
				snprintf(mask_str, sizeof(mask_str), "/%d ", rule->mask % 33);
		}
		else {
			c_strcpy(mask_str, " ", sizeof(mask_str));
		}

		c_strcat(cmd, mask_str, sizeof(cmd));
	}

	// protocol
	if (rule->protocol_type != 0) {
		c_strcat(cmd, "-p ", sizeof(cmd));

		if (rule->protocol_name[0] != 0) {
			c_strcat(cmd, rule->protocol_name, sizeof(cmd));
		}
		else {
			char proto[32];
			snprintf(proto, sizeof(proto), "%u", rule->protocol_number);
			c_strcat(cmd, proto, sizeof(cmd));
		}
		c_strcat(cmd, " ", sizeof(cmd));
	}

	// port
	if (rule->protocol_number != SYS_IPT_PROTOCOL_TCP ||
			rule->protocol_number != SYS_IPT_PROTOCOL_UDP)
	{
		if (rule->port_type[0] != 0) {
			if (chain == SYS_IPT_CHAIN_B_INPUT)
				c_strcat(cmd, "--sport ", sizeof(cmd));
			else   // output
				c_strcat(cmd, "--dport ", sizeof(cmd));

			if (rule->port_type[0] == 1) {
				c_strcat(cmd, rule->port_name[0], sizeof(cmd));
			}
			else {
				char num_str[32];
				snprintf(num_str, sizeof(num_str), "%u", rule->port_number[0]);
				c_strcat(cmd, num_str, sizeof(cmd));
			}


			if (rule->port_type[1] == 1) {
				c_strcat(cmd, ":", sizeof(cmd));
				c_strcat(cmd, rule->port_name[1], sizeof(cmd));
			}
			else if (rule->port_type[1] != 0) {
				char num_str[32];
				snprintf(num_str, sizeof(num_str), "%u", rule->port_number[1]);
				c_strcat(cmd, ":", sizeof(cmd));
				c_strcat(cmd, num_str, sizeof(cmd));
			}

			c_strcat(cmd, " ", sizeof(cmd));
		}
	}

	// ipset
	if (rule->addr_data_type == 2) {
		int len = c_strlen(cmd, sizeof(cmd));
		if (chain == SYS_IPT_CHAIN_B_INPUT)
			snprintf(&cmd[len], sizeof(cmd)-len, "-m set --match-set %s src ", rule->addr_data.ipset_name);
		else
			snprintf(&cmd[len], sizeof(cmd)-len, "-m set %s --match-set dst ", rule->addr_data.ipset_name);
	}

	// MAC
	if (rule->mac_type == 1) {
		if (chain == SYS_IPT_CHAIN_B_INPUT) {
			int len = c_strlen(cmd, sizeof(cmd));
			snprintf(&cmd[len], sizeof(cmd)-len, "-m mac --mac-source %02x:%02x:%02x:%02x:%02x:%02x ",
																					rule->mac_addr[0], rule->mac_addr[1], rule->mac_addr[2],
																					rule->mac_addr[3], rule->mac_addr[4], rule->mac_addr[5] );
		}
	}

	// target
	if (rule->target == SYS_IPT_TARGET_ACCEPT)
		c_strcat(cmd, "-j ACCEPT?", sizeof(cmd));
	else if (rule->target == SYS_IPT_TARGET_DROP)
		c_strcat(cmd, "-j DROP?", sizeof(cmd));
	else
		return FALSE;

	cmd_len = c_strlen(cmd, sizeof(cmd));
	if (cmd_len <= 0 || cmd[cmd_len-1] != '?') {
		grm_log_error("sys_ipt.c: Made command is invalid [%s]", cmd);
		return FALSE;
	}
	cmd[cmd_len-1] = 0;

	// actual calling the iptables command.
	grm_log_debug("sys_ipt.c: run command [%s]", cmd);
	done = sys_run_cmd_no_output(cmd, "sys_ipt");

	return done;
}

static gboolean _apply_rule (gboolean append, sys_ipt_rule *rule)
{
	gboolean done = FALSE;

	if (rule) {
		done = TRUE;
		if (rule->chain_set == 0) {
			done &= _make_and_run_cmd(append, SYS_IPT_CHAIN_B_INPUT,  rule);
			done &= _make_and_run_cmd(append, SYS_IPT_CHAIN_B_OUTPUT, rule);
		}
		else {
			if (rule->chain_set & SYS_IPT_CHAIN_B_INPUT)
				done &= _make_and_run_cmd(append, SYS_IPT_CHAIN_B_INPUT, rule);
			if (rule->chain_set & SYS_IPT_CHAIN_B_OUTPUT)
				done &= _make_and_run_cmd(append, SYS_IPT_CHAIN_B_OUTPUT, rule);
		}
	}

	return done;
}

gboolean	sys_ipt_insert_rule(sys_ipt_rule *rule)
{
	return _apply_rule (FALSE, rule);
}

gboolean	sys_ipt_append_rule(sys_ipt_rule *rule)
{
	return _apply_rule (TRUE, rule);
}

gboolean	sys_ipt_insert_all_drop_rule()
{
	sys_ipt_rule rule;

	sys_ipt_rule_init(&rule);
	rule.chain_set = SYS_IPT_CHAIN_B_INOUT;
	sys_ipt_rule_set_target (&rule, SYS_IPT_TARGET_DROP);

	return _apply_rule (FALSE, &rule);
}

gboolean	sys_ipt_append_all_drop_rule()
{
	sys_ipt_rule rule;

	sys_ipt_rule_init(&rule);
	rule.chain_set = SYS_IPT_CHAIN_B_INOUT;
	sys_ipt_rule_set_target (&rule, SYS_IPT_TARGET_DROP);

	return _apply_rule (TRUE, &rule);
}

gboolean	sys_ipt_insert_all_accept_rule()
{
	sys_ipt_rule rule;

	sys_ipt_rule_init(&rule);
	rule.chain_set = SYS_IPT_CHAIN_B_INOUT;

	sys_ipt_rule_set_target (&rule, SYS_IPT_TARGET_ACCEPT);

	return _apply_rule (FALSE, &rule);
}

gboolean	sys_ipt_append_all_accept_rule()
{
	sys_ipt_rule rule;

	sys_ipt_rule_init(&rule);
	rule.chain_set = SYS_IPT_CHAIN_B_INOUT;
	sys_ipt_rule_set_target (&rule, SYS_IPT_TARGET_ACCEPT);

	return _apply_rule (TRUE, &rule);
}


