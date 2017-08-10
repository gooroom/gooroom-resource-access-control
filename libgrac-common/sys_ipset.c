/*
 * sys_ipset.c
 *
 *  Created on: 2017. 7. 13.
 *      Author: user
 */

/**
  @file 	 	sys_ipset.c
  @brief		리눅스의 ipset 관련 함수
  @details	시스템 혹은 특정 라이브러리 의존적인 부분을 분리하는 목적의 래퍼 함수들이다.  \n
  				현재는 "ipset"  명령을 사용하였다.	\n
  				헤더파일 :  	sys_ipset.h	\n
  				라이브러리:	libgrac-common.so
 */

#include "sys_ipset.h"
#include "sys_etc.h"
#include "grm_log.h"
#include "cutility.h"

#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>

struct _sys_ipset_entry_t {
	// protocol
	int		protocol_type;	  // 0:not set(=all)  1:single
	int		protocol_number;
	char	protocol_name[32];

	// port [0]:from  [1]:to
	int		port_type  [2];				// 0:not set 1:normal(well-known)  2:not well-known
	int		port_number[2];
	char	port_name  [2][32];

	//address
	int	addr_data_type;		// 0:not set 1:IPv4 2:IPv6
	union {
		guint8	ip_v4[4];		// 127.0.10.1  --> [0]127 [1]:0 [2]:10 [3]:1
		guint16	ip_v6[8];
	} addr_data;
	char	addr_str[256];		    // dot style address

	// mask
	unsigned mask;				// 0 is no mask
};


// set handling
gboolean sys_ipset_create(char *name, int type, gboolean ipv6)
{
	gboolean done = FALSE;

	if (name) {
		char *set_type = NULL;
		char *family = NULL;
		switch (type) {
		case SYS_IPSET_TYPE_IP_ONLY :
			set_type = "hash:ip";
			break;
		case SYS_IPSET_TYPE_IP_PORT	:
			set_type = "hash:ip,port";
			break;
		default:
			grm_log_error("sys_ipt_create() : invalid type or address kind");
			break;
		}

		if (ipv6 == TRUE)
			family = "family:inet6";
		else
			family = "";

		if (set_type != NULL) {
			char	cmd[1024];
			if (ipv6 == TRUE)
				snprintf(cmd, sizeof(cmd), "ipset create %s %s %s", name, set_type, family);
			else
				snprintf(cmd, sizeof(cmd), "ipset create %s %s", name, set_type);

			done = sys_run_cmd_no_output(cmd, "sys_ipset");
		}
	}

	return done;
}

gboolean sys_ipset_delete(char *name)
{
	gboolean done = FALSE;

	if (name) {
		char	cmd[1024];
		snprintf(cmd, sizeof(cmd), "ipset destroy %s", name);

		done = sys_run_cmd_no_output(cmd, "sys_ipset");
	}

	return done;
}

gboolean sys_ipset_clear (char *name)
{
	gboolean done = FALSE;

	if (name) {
		char	cmd[1024];
		snprintf(cmd, sizeof(cmd), "ipset flush %s", name);

		done = sys_run_cmd_no_output(cmd, "sys_ipset");
	}

	return done;
}

gboolean sys_ipset_add_entry   (char *name, sys_ipset_entry_t *entry)
{
	gboolean done = FALSE;

	if (name && entry) {
		char	cmd[1024];
		snprintf(cmd, sizeof(cmd), "ipset add %s ", name);

		if (entry->addr_data_type != 0) {
			char tmp[256];
			c_strcat(cmd, entry->addr_str, sizeof(cmd));
			if (entry->mask) {
				snprintf(tmp, sizeof(tmp), "/%u", entry->mask);
				c_strcat(cmd, tmp, sizeof(cmd));
			}
			if (entry->port_type[0] != 0) {
				if (entry->protocol_type != 0)
					snprintf(tmp, sizeof(tmp), ",%s:", entry->protocol_name);
				else
					snprintf(tmp, sizeof(tmp), ",");
				c_strcat(cmd, tmp, sizeof(cmd));

				if (entry->protocol_type == 0) {
					snprintf(tmp, sizeof(tmp), "%u", entry->port_number[0]);
				}
				else if (entry->protocol_number == SYS_IPT_PROTOCOL_TCP || entry->protocol_number == SYS_IPT_PROTOCOL_UDP) {
					snprintf(tmp, sizeof(tmp), "%u", entry->port_number[0]);
				}
				else {
					snprintf(tmp, sizeof(tmp), "0");
				}
				c_strcat(cmd, tmp, sizeof(cmd));

				if (entry->port_type[1] != 0) {
					if (entry->protocol_type == 0 ||
					    entry->protocol_number == SYS_IPT_PROTOCOL_TCP || entry->protocol_number == SYS_IPT_PROTOCOL_UDP)
					{
						snprintf(tmp, sizeof(tmp), "-%u", entry->port_number[1]);
						c_strcat(cmd, tmp, sizeof(cmd));
					}
				}

				int n = c_strlen(cmd, sizeof(cmd));
				if (n < sizeof(cmd) -1) {
					done = sys_run_cmd_no_output(cmd, "sys_ipset");
					grm_log_debug("sys_ipset.c : cmd=[%s] : res = %d", cmd, (int)done);
				}
				else {
					grm_log_error("sys_ipset.c : May be too short buffer to make command ");
				}

			}
		}
	}

	return done;
}

//gboolean sys_ipset_delete_entry(char *name, sys_ipset_entry_t *entry);

//------------------------------------------------------------------------
// setting entry of set
//------------------------------------------------------------------------

void sys_ipset_entry_init(sys_ipset_entry_t *entry)
{
	if (entry) {
		c_memset(entry, 0, sizeof(sys_ipset_entry_t));
	}
}

gboolean sys_ipset_entry_get_type(sys_ipset_entry_t *entry, int *type, gboolean *ipv6)
{
	gboolean done = FALSE;

	if (entry && type && ipv6) {
		if (entry->addr_data_type != 0) {
			if (entry->port_type[0] == 0)
				*type = SYS_IPSET_TYPE_IP_ONLY;
			else
				*type = SYS_IPSET_TYPE_IP_PORT;

			if (entry->addr_data_type == 2) {
				*ipv6 = TRUE;
				done = TRUE;
			}
			else if (entry->addr_data_type == 1) {
				*ipv6 = FALSE;
				done = TRUE;
			}
		}
	}

	return done;
}

gboolean sys_ipset_entry_set_addr (sys_ipset_entry_t *entry, guint8  addr[4])
{
	gboolean done = FALSE;

	if (entry) {
		entry->addr_data_type = 1;
		c_memcpy(entry->addr_data.ip_v4, addr, sizeof(entry->addr_data.ip_v4));
		inet_ntop(AF_INET, entry->addr_data.ip_v4, entry->addr_str, sizeof(entry->addr_str) );

		done = TRUE;
	}

	return done;
}

gboolean sys_ipset_entry_set_addr6(sys_ipset_entry_t *entry, guint16 addr[8])
{
	gboolean done = FALSE;

	if (entry) {
		entry->addr_data_type = 2;
		c_memcpy(entry->addr_data.ip_v6, addr, sizeof(entry->addr_data.ip_v6));
		inet_ntop(AF_INET6, entry->addr_data.ip_v6, entry->addr_str, sizeof(entry->addr_str) );
		done = TRUE;
	}

	return done;
}

gboolean sys_ipset_entry_set_addr_str (sys_ipset_entry_t *entry, gchar *addr_str)
{
	gboolean done = FALSE;

	if (entry) {
		struct in_addr inaddr;
		int res = inet_pton(AF_INET, addr_str, &inaddr);
		if (res == 1) {
			entry->addr_data_type = 1;
			c_strcpy(entry->addr_str, addr_str, sizeof(entry->addr_str));
			c_memcpy(entry->addr_data.ip_v4, &inaddr, sizeof(entry->addr_data.ip_v4));
			done = TRUE;
		}
	}

	return done;
}

gboolean sys_ipset_entry_set_addr6_str(sys_ipset_entry_t *entry, gchar *addr_str)
{
	gboolean done = FALSE;

	if (entry) {
		struct in6_addr inaddr;
		int res = inet_pton(AF_INET6, addr_str, &inaddr);
		if (res == 1) {
			entry->addr_data_type = 2;
			c_strcpy(entry->addr_str, addr_str, sizeof(entry->addr_str));
			c_memcpy(entry->addr_data.ip_v6, &inaddr, sizeof(entry->addr_data.ip_v6));
			done = TRUE;
		}
	}

	return done;
}


gboolean sys_ipset_entry_set_mask (sys_ipset_entry_t *entry, guchar  mask)
{
	gboolean done = FALSE;

	if (entry) {
		entry->mask = mask;
		done = TRUE;
	}

	return done;
}

gboolean sys_ipset_entry_set_protocol(sys_ipset_entry_t *entry, int protocol)
{
	gboolean done = FALSE;

	if (entry) {
		if (protocol == -1) {
			entry->protocol_type = 0;
			entry->protocol_number = -1;
			entry->protocol_name[0] = 0;
			done = TRUE;
		}
		else {
			struct protoent *proto_ent = getprotobynumber(protocol);
			if (proto_ent) {
				entry->protocol_type = 1;
				entry->protocol_number = protocol;
				c_strcpy(entry->protocol_name, proto_ent->p_name, sizeof(entry->protocol_name));
				done = TRUE;
			}
		}
	}

	return done;
}

gboolean sys_ipset_entry_set_protocol_name(sys_ipset_entry_t *entry, char *proto_name)
{
	gboolean done = FALSE;

	if (entry) {
		if (c_strcmp(proto_name, "all", 4, -2) == 0 || c_strcmp(proto_name, "ALL", 4, -2) == 0 ) {
			entry->protocol_type = 0;
			entry->protocol_number = -1;
			entry->protocol_name[0] = 0;
			done = TRUE;
		}
		else {
			struct protoent *proto_ent = getprotobyname(proto_name);
			if (proto_ent) {
				entry->protocol_type = 1;
				entry->protocol_number = proto_ent->p_proto;
				c_strcpy(entry->protocol_name, proto_ent->p_name, sizeof(entry->protocol_name));
				done = TRUE;
			}
		}
	}

	return done;
}

static gboolean	_sys_ipset_set_port_number(sys_ipset_entry_t *entry, int idx, int port)
{
	gboolean done = FALSE;

	if (idx < 0 || idx > 1)
		return FALSE;

	if (entry) {
//  an only port without protocol is allowed for ipset (but not iptables)
//	if (entry->protocol_type != 1)
//		return FALSE;
//
//	if (entry->protocol_number != SYS_IPT_PROTOCOL_TCP &&
//				entry->protocol_number != SYS_IPT_PROTOCOL_UDP)
//		{
//			return FALSE;
//		}

		if (port == -1)
			port = 65535;

		struct servent *ent;
		ent = getservbyport(port, entry->protocol_name);
		if (ent) {
			entry->port_type[idx] = 1;
			entry->port_number[idx] = ent->s_port;
			c_strcpy(entry->port_name[idx], ent->s_name, sizeof(entry->port_name[idx]));
		}
		else {  // no error
			entry->port_type[idx] = 2;
			entry->port_number[idx] = port;
			entry->port_name[idx][0] = 0;
		}
		done = TRUE;
	}

	return done;
}

static gboolean	_sys_ipset_set_port_name(sys_ipset_entry_t *entry, int idx, char *name)
{
	gboolean done = FALSE;

	if (idx < 0 || idx > 1)
		return FALSE;

	if (c_strlen(name, 128) <= 0)
		return FALSE;

	if (entry) {
		//  an only port without protocol is allowed for ipset (but not iptables)
		//	if (entry->protocol_type != 1)
		//		return FALSE;
		//
		//	if (entry->protocol_number != SYS_IPT_PROTOCOL_TCP &&
		//				entry->protocol_number != SYS_IPT_PROTOCOL_UDP)
		//		{
		//			return FALSE;
		//		}

		struct servent *ent;
		ent = getservbyname(name, entry->protocol_name);
		if (ent) {
			entry->port_type[idx] = 1;
			entry->port_number[idx] = ent->s_port;
			c_strcpy(entry->port_name[idx], ent->s_name, sizeof(entry->port_name[idx]));
			done = TRUE;
		}
		else {  // error
			entry->port_type[idx] = 0;
			entry->port_name[idx][0] = 0;
		}
	}

	return done;
}

gboolean sys_ipset_entry_set_port (sys_ipset_entry_t *entry, int port)
{
	gboolean done = FALSE;

	if (entry) {
		done = _sys_ipset_set_port_number (entry, 0, port);
		entry->port_type[1] = 0;
		entry->port_name[1][0] = 0;
	}

	return done;
}

gboolean sys_ipset_entry_set_port2(sys_ipset_entry_t *entry, int from_port, int to_port)
{
	gboolean done = FALSE;

	if (entry) {
		done  = _sys_ipset_set_port_number (entry, 0, from_port);
		done &= _sys_ipset_set_port_number (entry, 1, to_port);
	}

	return done;
}

gboolean sys_ipset_entry_set_port_name (sys_ipset_entry_t *entry, gchar *name)
{
	gboolean done = FALSE;

	if (entry) {
		done = _sys_ipset_set_port_name (entry, 0, name);
		entry->port_type[1] = 0;
		entry->port_name[1][0] = 0;
	}

	return done;
}

gboolean sys_ipset_entry_set_port2_name(sys_ipset_entry_t *entry, gchar *from_name, gchar *to_name)
{
	gboolean done = FALSE;

	if (entry) {
		done  = _sys_ipset_set_port_name(entry, 0, from_name);
		done &= _sys_ipset_set_port_name(entry, 1, to_name);
	}

	return done;
}

