/*
 ============================================================================
 Name        : grac_access.c
 Author      : 
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */

/**
  @file 	 	grac_access.c
  @brief		리소스 접근 권한 정보
  @details	class GracAccess 정의 (GTK style의 c언어를 이용한 객체처리 사용) \n
  				각 리소스별 접근 권한은 map형 자료구조에 의해 문자렬로 된 키.데이터 형태로 관리된다 \n
  				키가 리소스 종류이고 데이터가 접근권한이 된다	\n
  				헤더파일 :  	grac_access.h	\n
  				라이브러리:	libgrac.so
 */

#include "grac_access.h"
#include "grac_access_attr.h"
#include "grac_apply.h"
#include "grac_map.h"
#include "grac_config.h"
#include "grm_log.h"
#include "cutility.h"
#include "sys_ipt.h"
#include "sys_ipset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <json-c/json.h>
#include <json-c/json_util.h>

#define GRAC_ACCESS_ALLOW  1
#define GRAC_ACCESS_DENY 	 2

typedef struct _GracNetworkAddress GracNetworkAddress;
struct _GracNetworkAddress {
	gchar		  *addr_str;
	gint		  kind;				// 0:unknown or not set 4:ipv4, 6:ipv6
	guint			mask;
};

typedef struct _GracNetworkAddrList GracNetworkAddrList;
struct _GracNetworkAddrList {
	gchar		  *id;
	GPtrArray *addr_array;			// GracNetworkAddress*
	gint		   addr_count;
};

typedef struct _GracNetworkAccess GracNetworkAccess;
struct _GracNetworkAccess {
	GracNetworkAddress		*addr;
	gchar		*addr_list_id;

	gchar		*mac_addr_str;

	int			perm_allow;
	int			perm_deny;

	int			dir_input;
	int			dir_output;

	GPtrArray *protocol_array;			// char*
	gint		   protocol_count;

	GPtrArray *port_array;			// char*
	gint		   port_count;
};

typedef struct _GracNetwork GracNetwork;
struct _GracNetwork {

	int	unlisted_perm_allow;
	int	unlisted_perm_deny;

	GPtrArray *addr_list_array;		// GracAccNetwork_AddrList;
	int	addr_list_count;

	GPtrArray *access_array;			// GracAccNetwork_Access;
	int	access_count;
};


struct _GracAccess
{
	GracNetwork	*network;

	// compatible to the UID based method (fixed target)		// 삭제 검토
	int	acc_printer;		// 0:not setting, set 'y' or 'n'
	int	acc_network;		// 0:not setting, set 'y' or 'n'
	int	acc_usb;				// 0:not setting, set 'y' or 'n'
	int	acc_capture;		// 0:not setting, set 'y' or 'n'
	int	acc_harddisk;	  // 0:not setting, set 'y' or 'n'

	// new method - (attr, perm) map
	gchar	 *acc_name;			// internal usages
	GracMap *acc_attr_map;	// {attr,perm}

	gchar	*perm_str;
	gint	perm_id;
};


static GracNetworkAddress* _grac_network_address_alloc()
{
	GracNetworkAddress* network_addr = malloc(sizeof(GracNetworkAddress));
	if (network_addr) {
		memset(network_addr, 0, sizeof(GracNetworkAddress));
	}
	return network_addr;
}


static void _grac_network_address_free(GracNetworkAddress **pNetworkAddr)
{
	if (pNetworkAddr) {
		GracNetworkAddress* network_addr = *pNetworkAddr;
		if (network_addr) {
			c_free(&network_addr->addr_str);
			free(network_addr);
		}
		*pNetworkAddr = NULL;
	}
}


static void _grac_network_clear(GracNetwork *network)
{
	if (network) {
		if (network->addr_list_array) {
			GracNetworkAddrList *addr_list;
			int	idx, i;
			for (idx=0; idx < network->addr_list_count; idx++) {
				addr_list = g_ptr_array_index (network->addr_list_array, idx);
				if (addr_list == NULL)
					continue;
				c_free(&addr_list->id);
				for (i=0; i < addr_list->addr_count; i++) {
					GracNetworkAddress* addr = g_ptr_array_index (addr_list->addr_array, i);
					if (addr) {
						c_free(&addr->addr_str);
						free(addr);
					}
				}
				if (addr_list->addr_array)
					g_ptr_array_free(addr_list->addr_array, TRUE);
			}
			g_ptr_array_free(network->addr_list_array, TRUE);

			network->addr_list_array = NULL;
			network->addr_list_count = 0;
		}

		if (network->access_array) {
			GracNetworkAccess *access;
			int idx, i;
			for (idx=0; idx<network->access_count; idx++) {
				access = g_ptr_array_index (network->access_array, idx);
				if (access == NULL)
					continue;

				_grac_network_address_free(&access->addr);
				c_free(&access->addr_list_id);
				c_free(&access->mac_addr_str);

				for (i=0; i<access->protocol_count; i++) {
					gchar *proto = g_ptr_array_index (access->protocol_array, i);
					c_free(&proto);
				}
				if (access->protocol_array)
					g_ptr_array_free(access->protocol_array, TRUE);

				for (i=0; i<access->port_count; i++) {
					gchar *port = g_ptr_array_index (access->port_array, i);
					c_free(&port);
				}
				if (access->port_array)
					g_ptr_array_free(access->port_array, TRUE);
			}

			network->access_array = NULL;
			network->access_count = 0;
		}

		network->unlisted_perm_allow = 0;
		network->unlisted_perm_deny = 0;

	}

}

static void _grac_network_free(GracNetwork **pNetwork)
{
	if (pNetwork) {
		GracNetwork* network = *pNetwork;
		if (network) {
			_grac_network_clear(network);

			free(network);
		}
		*pNetwork = NULL;
	}
}

static GracNetwork* _grac_network_alloc()
{
	GracNetwork* network = malloc(sizeof(GracNetwork));
	if (network) {
		memset(network, 0, sizeof(GracNetwork));
		network->unlisted_perm_deny = 1;

	}
	return network;
}

static GracNetworkAddrList* _grac_network_add_addr_list(GracNetwork* network)
{
	GracNetworkAddrList *list;

	if (network->addr_list_array == NULL) {
		network->addr_list_array = g_ptr_array_new();
		if (network->addr_list_array == NULL) {
			grm_log_error("grac_access.c : can't alloc for addr_list_array");
			return NULL;
		}
	}

	list = malloc(sizeof(GracNetworkAddrList));
	if (list) {
		c_memset(list, 0, sizeof(GracNetworkAddrList));
		g_ptr_array_add(network->addr_list_array, list);
		network->addr_list_count++;
	}

	return list;
}

static GracNetworkAddrList* _grac_network_find_addr_list(GracNetwork* network, gchar *list_id)
{
	GracNetworkAddrList *list = NULL;

	if (network && list_id) {
		int	idx;

		for (idx=0; idx<network->addr_list_count; idx++) {
			GracNetworkAddrList *tmp = g_ptr_array_index (network->addr_list_array, idx);
			if (tmp && c_strimatch(tmp->id, list_id)) {
				list = tmp;
				break;
			}
		}
	}

	return list;
}


static gboolean _grac_network_addrees_set_addr(GracNetworkAddress* net_addr, gchar* addr_str)
{
	gboolean done = FALSE;

	if (net_addr && addr_str) {

		gchar *new_addr_str = c_strdup(addr_str, 65535);
		if (new_addr_str == NULL) {
			grm_log_error("grac_access.c (%d) : out of memory to store address", __LINE__);
			return FALSE;
		}

		net_addr->addr_str = new_addr_str;
		net_addr->mask = 0;

		char *ptr = c_strchr(new_addr_str, '/', 65535);
		if (ptr != NULL) {
			int	len, num;
			char *ptr_mask = ptr + 1;
			len = c_get_number(ptr_mask, 65535, &num);
			if (len > 0 && ptr_mask[len] == 0) {
				net_addr->mask = num;
				*ptr = 0;		// delete mask
  			}
		}

		// ipv6, ipv4
		struct addrinfo hints;
		struct addrinfo *info_list;

		memset(&hints, 0, sizeof(hints));
		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		int err = getaddrinfo(net_addr->addr_str, NULL, NULL, &info_list);
		if (err != 0) {
			grm_log_warning("Can't get address info : %s", net_addr->addr_str);
			net_addr->kind = 0;
			done = TRUE;
		}
		else {
			struct addrinfo *iter;
			//struct sockaddr_in*  sin;
			//struct sockaddr_in6* sin6;
			//char	 ip_addr[256];

			// for (iter=info_list; iter!=NULL; iter=iter->ai_next)
			iter = info_list;
			if (iter->ai_family == AF_INET) {
				net_addr->kind = 4;
				//sin = (void*)iter->ai_addr;
				//inet_ntop(iter->ai_family, &sin->sin_addr, ip_addr, sizeof(ip_addr));
				//grm_log_debug("IPv4 - %s -> %s", net_addr->addr_str, ip_addr);
			}
			else if (iter->ai_family == AF_INET6) {
				net_addr->kind = 6;
				//sin6 = (void*)iter->ai_addr;
				//inet_ntop(iter->ai_family, &sin6->sin6_addr, ip_addr, sizeof(ip_addr));
				//grm_log_debug("IPv6 - %s -> %s", net_addr->addr_str, ip_addr);
			}
			else {
				net_addr->kind = 0;
			}

			done = TRUE;
		}
	}

	return done;
}

static gboolean _grac_network_addr_list_add_addr(GracNetworkAddrList* list, gchar* addr_str)
{
	gboolean done = FALSE;

	if (list && addr_str) {

		if (list->addr_array == NULL) {
			list->addr_array = g_ptr_array_new();
			if (list->addr_array == NULL) {
				grm_log_error("grac_access.c : can't alloc address array of addr_list");
				return FALSE;
			}
		}

		GracNetworkAddress* new_addr = _grac_network_address_alloc();
		if (new_addr == NULL) {
			grm_log_error("grac_access.c : can't alloc to add address into addr_array");
		}
		else {
			done = _grac_network_addrees_set_addr(new_addr, addr_str);
			if (done == FALSE) {
				grm_log_error("grac_access.c : can't set address : %s", addr_str);
				_grac_network_address_free(&new_addr);
			}
			else {
				g_ptr_array_add(list->addr_array, new_addr);
				list->addr_count++;
				done = TRUE;
			}
		}
	}

	return done;
}

static gboolean _grac_network_addr_list_set_id(GracNetworkAddrList* list, gchar* id)
{
	gboolean done = FALSE;

	if (list && id) {

		gchar *add_id = c_strdup(id, 65535);
		if (add_id == NULL) {
			grm_log_error("grac_access.c : can't alloc to add the addr_list ID");
		}
		else {
			c_free(&list->id);
			list->id = add_id;
			done = TRUE;
		}

	}

	return done;
}


static GracNetworkAccess* _grac_network_add_access(GracNetwork* network)
{
	GracNetworkAccess *access;

	if (network->access_array == NULL) {
		network->access_array = g_ptr_array_new();
		if (network->access_array == NULL) {
			grm_log_error("grac_access.c : can't alloc for access_array");
			return NULL;
		}
	}

	access = malloc(sizeof(GracNetworkAccess));
	if (access) {
		c_memset(access, 0, sizeof(GracNetworkAccess));
		g_ptr_array_add(network->access_array, access);
		network->access_count++;
	}

	return access;
}

static gboolean _grac_network_access_set_address(GracNetworkAccess* net_access, gchar *addr_str)
{
	gboolean done = FALSE;

	if (net_access && addr_str) {
		GracNetworkAddress* new_addr = _grac_network_address_alloc();
		if (new_addr == NULL) {
			grm_log_error("grac_access.c : can't alloc to set the address of access");
		}
		else {
			done = _grac_network_addrees_set_addr(new_addr, addr_str);
			if (done == FALSE) {
				grm_log_error("grac_access.c : can't set address : %s", addr_str);
				_grac_network_address_free(&new_addr);
			}
			else {
				c_free(&net_access->addr_list_id);
				_grac_network_address_free(&net_access->addr);

				net_access->addr = new_addr;
			}
		}
	}

	return done;
}

static gboolean _grac_network_access_set_list_id(GracNetworkAccess* net_access, gchar *id_str)
{
	gboolean done = FALSE;

	if (net_access && id_str) {

		gchar *add_id = c_strdup(id_str, 65535);
		if (add_id == NULL) {
			grm_log_error("grac_access.c : can't alloc to set the addr_list ID of access");
		}
		else {
			c_free(&net_access->addr_list_id);
			_grac_network_address_free(&net_access->addr);

			net_access->addr_list_id = add_id;
			done = TRUE;
		}
	}

	return done;
}

static gboolean _grac_network_access_set_mac_addr(GracNetworkAccess* net_access, gchar *mac_str)
{
	gboolean done = FALSE;

	if (net_access && mac_str) {

		gchar *add_mac = c_strdup(mac_str, 65535);
		if (add_mac == NULL) {
			grm_log_error("grac_access.c : can't alloc to set the mac address of access");
		}
		else {
			net_access->mac_addr_str = add_mac;
			done = TRUE;
		}
	}

	return done;
}

static gboolean _grac_network_access_add_protocol(GracNetworkAccess* net_access, gchar* proto)
{
	gboolean done = FALSE;

	if (net_access && proto) {

		if (net_access->protocol_array == NULL) {
			net_access->protocol_array = g_ptr_array_new();
			if (net_access->protocol_array == NULL) {
				grm_log_error("grac_access.c : can't alloc protocol array of access");
				return FALSE;
			}
		}

		gchar *add_proto = c_strdup(proto, 65535);
		if (add_proto == NULL) {
			grm_log_error("grac_access.c : can't alloc to add protocol");
		}
		else {
			g_ptr_array_add(net_access->protocol_array, add_proto);
			net_access->protocol_count++;
			done = TRUE;
		}

	}

	return done;
}

static gboolean _grac_network_access_add_port(GracNetworkAccess* net_access, gchar* port)
{
	gboolean done = FALSE;

	if (net_access && port) {

		if (net_access->port_array == NULL) {
			net_access->port_array = g_ptr_array_new();
			if (net_access->port_array == NULL) {
				grm_log_error("grac_access.c : can't alloc port array of access");
				return FALSE;
			}
		}

		gchar *add_port = c_strdup(port, 65535);
		if (add_port == NULL) {
			grm_log_error("grac_access.c : can't alloc to add port");
		}
		else {
			g_ptr_array_add(net_access->port_array, add_port);
			net_access->port_count++;
			done = TRUE;
		}

	}

	return done;
}



/**
 @brief   GracAccess  객체 생성
 @details
      	객체 지향형 언어의 생성자에 해당한다.
 @return GracAccess* 생성된 객체 주소
 */
GracAccess* grac_access_alloc()
{
	GracAccess *acc = malloc(sizeof(GracAccess));
	if (acc) {
		memset(acc, 0, sizeof(GracAccess));
		acc->network = _grac_network_alloc();
		if (acc->network == NULL) {
			grm_log_debug("grac_access_alloc() : out of memory");
			grac_access_free(&acc);
			return NULL;
		}

		acc->acc_attr_map = grac_map_alloc();
		if (acc->acc_attr_map == NULL) {
			grm_log_debug("grac_access_alloc() : out of memory");
			grac_access_free(&acc);
			return NULL;
		}
	}

	return acc;
}

/**
 @brief   GracAccess 객체 해제
 @details
      객체 지향형 언어의 소멸자에  해당한다. 단 자동으로 불려지지 않고 필요한 시점에서 직접 호출하여야 한다.\n
      해제된  객체 주소값으로 0으로 설정하기 위해 이중 포인터를 사용한다.
 @param [in,out]  pAcc  해제하고자 하는 객체 주소를 담는 변수의 주소 (double pointer)
 */
void grac_access_free(GracAccess **pAcc)
{
	if (pAcc != NULL && *pAcc != NULL) {
		GracAccess *acc = *pAcc;

		_grac_network_free(&acc->network);

		c_free(&acc->acc_name);
		grac_map_free(&acc->acc_attr_map);

		c_free(&acc->perm_str);

		free(acc);

		*pAcc = NULL;
	}
}

void grac_access_clear(GracAccess *access)
{
	if (access) {
		_grac_network_clear(access->network);

		grac_map_clear(access->acc_attr_map);
	}
}

static gboolean _grac_access_load_json_addr_list_one(GracAccess *access, json_object *jobj_addr_list)
{
	if (access == NULL || jobj_addr_list == NULL)
		return FALSE;

	GracNetworkAddrList *list;
	list = _grac_network_add_addr_list(access->network);
	if (list == NULL) {
		grm_log_error("grac_access.c : can't alloc for addr_list");
		return FALSE;
	}

	json_object_object_foreach(jobj_addr_list, key, val) {
		if (c_strmatch(key, "id")) {
			char *id_str = (char*)json_object_get_string(val);
			_grac_network_addr_list_set_id(list, id_str);
		}
		else if (c_strmatch(key, "entry")) {
			int  count = json_object_array_length(val);
			int  idx;
			char *addr;
			for (idx=0; idx<count; idx++) {
				json_object* jentry = json_object_array_get_idx(val, idx);
				addr = (char*)json_object_get_string(jentry);
				_grac_network_addr_list_add_addr(list, addr);
			}
		}
		else {
			grm_log_debug("grac_access.c : unknown key [%s] in <network/addr_list>", key);
		}
	} // foreach

	return TRUE;
}

static gboolean _grac_access_load_json_addr_list(GracAccess *grac_access, json_object *jobj_addr_list)
{
	gboolean done = FALSE;

	if (grac_access == NULL || jobj_addr_list == NULL)
		return FALSE;

	json_type jtype = json_object_get_type(jobj_addr_list);
	if (jtype == json_type_object) {
		return _grac_access_load_json_addr_list_one(grac_access, jobj_addr_list);

	}
	else if (jtype == json_type_array) {
		int  count = json_object_array_length(jobj_addr_list);
		int  idx;
		json_object*	jobj_entry;
		done = TRUE;
		for (idx=0; idx<count; idx++) {
			jobj_entry = json_object_array_get_idx(jobj_addr_list, idx);
			done &= _grac_access_load_json_addr_list_one(grac_access, jobj_entry);
		}
	}
	else {
		grm_log_error("_grac_access_load_json_addr_list() : invalid json type for access");
		return FALSE;
	}

	return done;
}


static gboolean _grac_access_load_json_access_one(GracAccess *grac_access, json_object *jobj_access)
{
	if (grac_access == NULL || jobj_access == NULL)
		return FALSE;

	GracNetworkAccess *net_access;
	net_access = _grac_network_add_access(grac_access->network);
	if (net_access == NULL) {
		grm_log_error("grac_access.c : can't alloc for access in <network>");
		return FALSE;
	}

	json_object_object_foreach(jobj_access, key, val) {
		if (c_strmatch(key, "address")) {
			char *addr_str = (char*)json_object_get_string(val);
			_grac_network_access_set_address(net_access, addr_str);
		}
		else if (c_strmatch(key, "addr_list")) {
			char *id_str = (char*)json_object_get_string(val);
			_grac_network_access_set_list_id(net_access, id_str);
		}
		else if (c_strmatch(key, "mac_addr")) {
			char *mac_str = (char*)json_object_get_string(val);
			_grac_network_access_set_mac_addr(net_access, mac_str);
		}
		else if (c_strmatch(key, "permission")) {
			char *perm_str = (char*)json_object_get_string(val);
			if (c_strimatch(perm_str, "ALLOW")) {
				net_access->perm_allow = 1;
				net_access->perm_deny = 0;
			}
			else if (c_strimatch(perm_str, "DENY")) {
				net_access->perm_allow = 0;
				net_access->perm_deny = 1;
			}
			else {
				grm_log_debug("grac_access.c : unknown permission of access in <network>");
			}
		}
		else if (c_strmatch(key, "direction")) {
			char *dir_str = (char*)json_object_get_string(val);
			if (c_strimatch(dir_str, "INPUT") || c_strimatch(dir_str, "INBOUND")) {
				net_access->dir_input = 1;
			}
			else if (c_strimatch(dir_str, "OUTPUT") || c_strimatch(dir_str, "OUTBOUND")) {
				net_access->dir_output = 1;
			}
			else if (c_strimatch(dir_str, "INOUT")) {
				net_access->dir_input = 1;
				net_access->dir_output = 1;
			}
			else {
				grm_log_debug("grac_access.c : unknown permission of access in <network>");
			}

		}
		else if (c_strmatch(key, "protocol")) {
			int  count = json_object_array_length(val);
			int  idx;
			char *proto_str;
			for (idx=0; idx<count; idx++) {
				json_object* jentry = json_object_array_get_idx(val, idx);
				proto_str = (char*)json_object_get_string(jentry);
				_grac_network_access_add_protocol(net_access, proto_str);
			}
		}
		else if (c_strmatch(key, "port")) {
			int  count = json_object_array_length(val);
			int  idx;
			char *port_str;
			for (idx=0; idx<count; idx++) {
				json_object* jentry = json_object_array_get_idx(val, idx);
				port_str = (char*)json_object_get_string(jentry);
				_grac_network_access_add_port(net_access, port_str);
			}
		}
		else {
			grm_log_debug("grac_access.c : unknown key [%s] in <network/access>", key);
		}
	} // foreach

	return TRUE;
}

static gboolean _grac_access_load_json_access(GracAccess *grac_access, json_object *jobj_access)
{
	gboolean done = FALSE;

	if (grac_access == NULL || jobj_access == NULL)
		return FALSE;

	json_type jtype = json_object_get_type(jobj_access);
	if (jtype == json_type_object) {
		return _grac_access_load_json_access_one(grac_access, jobj_access);

	}
	else if (jtype == json_type_array) {
		int  count = json_object_array_length(jobj_access);
		int  idx;
		json_object*	jobj_entry;
		done = TRUE;
		for (idx=0; idx<count; idx++) {
			jobj_entry = json_object_array_get_idx(jobj_access, idx);
			done &= _grac_access_load_json_access_one(grac_access, jobj_entry);
		}
	}
	else {
		grm_log_error("_grac_access_load_json_access() : invalid json type for access");
		return FALSE;
	}

	return done;
}

static gboolean _grac_access_load_json(GracAccess *access, gchar* path)
{
	gboolean done = TRUE;
	json_object *jroot = json_object_from_file(path);

	if (jroot == NULL)
		return FALSE;

	grac_access_clear(access);

	{   // old compatible
		json_object *jobj;
		json_bool res = json_object_object_get_ex(jroot, "simple_format", &jobj);
		if (res) {

			json_object_object_foreach(jobj, key, val) {
				//json_type jtype_val = json_object_get_type(val);
				grac_access_put_attr(access, key, (char*)json_object_get_string(val));
			} // foreach
		}
	}


	json_object *jobj_network;
	json_bool res = json_object_object_get_ex(jroot, "network", &jobj_network);
	if (res == 0) {
		grm_log_debug("grac_access.c : No network access data");
		done = FALSE;
	}

	if (done) {
		json_object_object_foreach(jobj_network, key, val) {
			if (c_strmatch(key, "unlisted")) {
				char *perm = (char*)json_object_get_string(val);
				if (c_strimatch(perm, "ALLOW")) {
					access->network->unlisted_perm_allow = 1;
					access->network->unlisted_perm_deny = 0;
				}
				else if (c_strimatch(perm, "DENY")) {
					access->network->unlisted_perm_allow = 0;
					access->network->unlisted_perm_deny = 1;
				}
				else {
					grm_log_debug("grac_access.c : unknown permission for unlisted in <network>");
				}
			}
			else if (c_strmatch(key, "addr_list")) {
				_grac_access_load_json_addr_list(access, val);
			}
			else if (c_strmatch(key, "access")) {
				_grac_access_load_json_access(access, val);
			}
			else {
				grm_log_debug("grac_access.c : unknown key [%s] in <network>", key);
			}
		} // foreach
	}

	json_object_put(jroot);

	return done;
}

void _grac_access_dump (GracAccess *data)		// only to test
{
	if (data) {
		printf("<<<Network>>>\n");
		if (data->network == NULL) {
			printf("-- no data --\n");
		}
		else {
			GracNetwork *net = data->network;
			int	i;
			printf("unlisted-perm : %d %d\n", net->unlisted_perm_allow, net->unlisted_perm_deny);
			printf("count of addr_list : %d\n", net->addr_list_count);
			for (i=0; i<net->addr_list_count; i++) {
				GracNetworkAddrList *list = g_ptr_array_index (net->addr_list_array, i);
				if (list) {
					printf("%d: %s, address:%d\n", i, list->id, list->addr_count);
					int idx;
					for (idx=0; idx<list->addr_count; idx++) {
						GracNetworkAddress* net_addr = g_ptr_array_index (list->addr_array, idx);
						printf("\t%d-%s kind(%d) mask(%d)\n", idx, net_addr->addr_str, net_addr->kind, net_addr->mask);
					}
				}
			}

			printf("count of access : %d\n", net->access_count);
			for (i=0; i<net->access_count; i++) {
				GracNetworkAccess *acc = g_ptr_array_index (net->access_array, i);
				if (acc) {
					int idx;
					if (acc->addr == NULL)
						printf("%d: address  :%s\n", i, "NULL");
					else
						printf("%d: address  :%s kind(%d) mask(%d)\n", i, acc->addr->addr_str, acc->addr->kind, acc->addr->mask);
					printf("%d: addr_list:%s\n", i, acc->addr_list_id ? acc->addr_list_id : "");
					printf("%d: mac_addr :%s\n", i, acc->mac_addr_str ? acc->mac_addr_str : "");
					printf("%d: permission: %d,%d\n", i, acc->perm_allow, acc->perm_deny);
					printf("%d: direction: %d,%d\n", i, acc->dir_input, acc->dir_output);
					printf("%d: protocol :", i);
					for (idx=0; idx<acc->protocol_count; idx++)
						printf(" %s", (char*)g_ptr_array_index (acc->protocol_array, idx));
					printf("\n");
					printf("%d: port :", i);
					for (idx=0; idx<acc->port_count; idx++)
						printf(" %s", (char*)g_ptr_array_index (acc->port_array, idx));
					printf("\n");
				}
			}
		}
	}
}


gboolean grac_access_load (GracAccess *data, gchar* path)
{
	gboolean done = FALSE;

	if (data) {
		if (path == NULL)
			path = (char*)grac_config_path_access();

		if (path) {
			done = _grac_access_load_json(data, path);
		}
	}

	if (done)
		_grac_access_dump (data);

	return done;
}

static gboolean _grac_network_access_apply_addr(GracNetworkAccess *netacc, GracNetworkAddress *addr)
{
	char *func = "_grac_access_apply_network_addr()";
	gboolean done = TRUE;
	gboolean setB = TRUE;
	gboolean appB;
	sys_ipt_rule 	*ipt_rule;
	char *log_addr;

	if (addr == NULL)
		log_addr = "All Addresses";
	else
		log_addr = addr->addr_str;

	if (netacc == NULL)
		return FALSE;

	ipt_rule = sys_ipt_rule_alloc();
	if (ipt_rule == NULL) {
		grm_log_error("%s : out of memory", func);
		return FALSE;
	}

	sys_ipt_rule_init(ipt_rule);

	// address
	if (addr != NULL) {
		if (addr->kind == 6)
			setB &= sys_ipt_rule_set_addr6_str(ipt_rule, addr->addr_str);
		else
			setB &= sys_ipt_rule_set_addr_str (ipt_rule, addr->addr_str);

		if (addr->mask)
			setB &= sys_ipt_rule_set_mask (ipt_rule, addr->mask);
	}

	// mac address
	if (netacc->mac_addr_str)
		setB &= sys_ipt_rule_set_mac_addr_str(ipt_rule, netacc->mac_addr_str);

	// direction
	if (netacc->dir_input == 1 && netacc->dir_output == 1)
		setB &= sys_ipt_rule_set_chain (ipt_rule, SYS_IPT_CHAIN_B_INOUT);
	else if (netacc->dir_input)
		setB &= sys_ipt_rule_set_chain (ipt_rule, SYS_IPT_CHAIN_B_INPUT);
	else if (netacc->dir_output == 1)
		setB &= sys_ipt_rule_set_chain (ipt_rule, SYS_IPT_CHAIN_B_OUTPUT);
	else
		setB &= sys_ipt_rule_set_chain (ipt_rule, SYS_IPT_CHAIN_B_INOUT);

	// permission
	if (netacc->perm_allow)
		setB &= sys_ipt_rule_set_target (ipt_rule, SYS_IPT_TARGET_ACCEPT);
	else if (netacc->perm_deny)
		setB &= sys_ipt_rule_set_target (ipt_rule, SYS_IPT_TARGET_DROP);
	else
		setB &= sys_ipt_rule_set_target (ipt_rule, SYS_IPT_TARGET_DROP);

	if (setB == FALSE) {
		grm_log_error("%s : making ipt_rule : %s", func, log_addr);
		return FALSE;
	}
	done &= setB;

	// protocol && port
	if (netacc->protocol_count == 0) {
		if (netacc->port_count == 0) {
			appB = sys_ipt_append_rule(ipt_rule);
			if (appB == FALSE) {
				grm_log_error("%s : applying ipt_rule : %s, all protocols, all ports", func, log_addr);
			}
			done &= appB;
		}
		else {
			int port_idx;
			for (port_idx=0; port_idx < netacc->port_count; port_idx++) {
				gchar *port_str = g_ptr_array_index(netacc->port_array, port_idx);
				setB = sys_ipt_rule_set_port_str(ipt_rule, port_str);
				if (setB == FALSE) {
					grm_log_error("%s : making ipt_rule : %s, all protocols, %s", func, log_addr, port_str);
					continue;
				}
				done &= setB;
				appB = sys_ipt_append_rule(ipt_rule);
				if (appB == FALSE) {
					grm_log_error("%s : applying ipt_rule : %s, all protocols, %s", func, log_addr, port_str);
				}
				done &= appB;
			}
		}
	}
	else {
		int protocol_idx;
		for (protocol_idx=0; protocol_idx<netacc->protocol_count; protocol_idx++) {
			gchar *protocol_name = g_ptr_array_index(netacc->protocol_array, protocol_idx);
			setB = sys_ipt_rule_set_protocol_name(ipt_rule, protocol_name);
			if (setB == FALSE) {
				grm_log_error("%s : making ipt_rule : %s, %s", func, log_addr, protocol_name);
				continue;
			}
			if (netacc->port_count == 0) {
				appB = sys_ipt_append_rule(ipt_rule);
				if (appB == FALSE) {
					grm_log_error("%s : applying ipt_rule : %s, %s, all ports", func, log_addr, protocol_name);
				}
				done &= appB;
			}
			else {
				int port_idx;
				for (port_idx=0; port_idx < netacc->port_count; port_idx++) {
					gchar *port_str = g_ptr_array_index(netacc->port_array, port_idx);
					if (c_strimatch(port_str, "tcp") || c_strimatch(port_str, "udp")) {
						setB = sys_ipt_rule_set_port_str(ipt_rule, port_str);
						if (setB == FALSE) {
							grm_log_error("%s : making ipt_rule : %s, %s, %s", func, log_addr, protocol_name, port_str);
							continue;
						}
					}
					appB = sys_ipt_append_rule(ipt_rule);
					if (appB == FALSE) {
						grm_log_error("%s : applying ipt_rule : %s, %s, %s", func, log_addr, protocol_name, port_str);
					}
					done &= appB;
				}
			}
		}
	}

	sys_ipt_rule_free(&ipt_rule);

	return done;
}


static gboolean _grac_network_apply(GracNetwork *network)
{
	gboolean done = TRUE;
	char *func = "_grac_access_apply_network()";

	if (network == NULL) {
		// set deny
		done &= sys_ipt_clear_all();
		done &= sys_ipt_insert_all_drop_rule();
		return done;
	}

	done &= sys_ipt_clear_all();

	int	idx_net;
	for (idx_net=0; idx_net < network->access_count; idx_net++) {
		GracNetworkAccess *net_access;
		net_access = g_ptr_array_index (network->access_array, idx_net);
		if (net_access == NULL)
			continue;

		if (net_access->addr != NULL) {
			done &= _grac_network_access_apply_addr(net_access, net_access->addr);
		}
		else if (c_strlen(net_access->addr_list_id, 1) > 0) {
			GracNetworkAddrList* list = _grac_network_find_addr_list(network, net_access->addr_list_id);
			if (list == NULL) {
				grm_log_error("%s : not found the address list : %s", func, net_access->addr_list_id);
			}
			else {
				int	idx_addr;
				for (idx_addr = 0; idx_addr < list->addr_count; idx_addr++) {
					GracNetworkAddress *addr = g_ptr_array_index(list->addr_array, idx_addr);
					if (addr)
						done &= _grac_network_access_apply_addr(net_access, addr);
				}
			}
		}
		else {	// all address
			done &= _grac_network_access_apply_addr(net_access, NULL);
		}
	}

	if (network->unlisted_perm_allow == 1) {
		done &= sys_ipt_append_all_accept_rule();
	}
	else {	// not check the unlisted_perm_deny, because default permission is deny.
		done &= sys_ipt_append_all_drop_rule();
	}

	return done;
}

gboolean grac_access_apply (GracAccess *grac_access)
{
	gboolean done = TRUE;

	if (grac_access) {
		done &= _grac_network_apply(grac_access->network);
	}

	return done;
}

// 구버전 대응 - 매체제오방식 확정후 삭제 검토
gboolean grac_access_apply_by_user (GracAccess *grac_access, gchar* username)
{
	gboolean done = TRUE;

	if (grac_access) {
		done = grac_apply_access_by_user(grac_access, username);
	}

	return done;
}

static gboolean _grac_acc_set_attr_perm(GracAccess *access, gchar* attr, gchar *perm, gboolean adj);
static void _grac_acc_set_allow_by_perm(GracAccess *access, gchar* attr, gchar *perm);


// if existed, change permission
static gboolean _grac_acc_put_attr(GracAccess *access, gchar* attr, gchar* perm, gboolean adj)
{
	gboolean ret = FALSE;

	if (access && attr && perm) {
		if (grac_access_find_attr(access, attr) == TRUE)
			ret = _grac_acc_set_attr_perm(access, attr, perm, FALSE);
		else
			ret = grac_map_set_data(access->acc_attr_map, attr, perm);
		if (adj)
			_grac_acc_set_allow_by_perm(access, attr, perm);
	}
	else {
		grm_log_debug("grac_access_add_attr() : invalid argument");
	}

	return ret;
}


/**
  @brief   하나의 속성(리소스)에 대한 접근 권한을 추가한다.
  @details
  		이미 존재하는 경우는 권한을 변경한다.
  @param [in]  access	GracAccess 객체주소
  @param [in]  attr		속성 (리소스 종류등)
  @param [in]  perm		권한 (접근허가등)
  @return gboolean 성공여부
 */
gboolean grac_access_put_attr(GracAccess *access, gchar* attr, gchar* perm)
{
	return _grac_acc_put_attr(access, attr, perm, TRUE);
}


/**
  @brief   지정된 속성( 리소스)에 대한 접근 권한을 삭제한다.
  @details
  		이미 존재하는 경우는 권한을 변경한다.
  @param [in]  access	GracAccess 객체주소
  @param [in]  attr		속성 (리소스 종류등)
  @return gboolean 성공여부
 */
gboolean grac_access_del_attr(GracAccess *access, gchar* attr)
{
	gboolean ret = FALSE;

	if (access && attr) {
		grac_map_del_data(access->acc_attr_map, attr);
		ret = TRUE;
	}
	else {
		grm_log_debug("grac_access_del_attr() : invalid argument");
	}
	return ret;
}

/**
  @brief   지정된 속성( 리소스)이 존재하는지 확인한다.
  @param [in]  access	GracAccess 객체주소
  @param [in]  attr		속성 (리소스 종류등)
  @return gboolean 존재 여부
 */
gboolean grac_access_find_attr(GracAccess *access, gchar* attr)
{
	gboolean ret = FALSE;
	if (access && attr) {
		if (grac_map_get_data(access->acc_attr_map, attr) != NULL)
			ret = TRUE;
	}
	else {
		grm_log_debug("grac_access_find_attr() : invalid argument");
	}
	return ret;
}

/**
  @brief   속성( 리소스) 목록중 최초의 것을 구한다.
  @details
  		이 후 grac_access_find_next_attr()를 반복적으로 호출하여 모든 속성을 구할 수 있다.
  @param [in]  access	GracAccess 객체주소
  @return gchar*	속성명,  없으면 NULL
 */
gchar* grac_access_find_first_attr(GracAccess *access)
{
	gchar *attr = NULL;

	if (access) {
		attr = (gchar*)grac_map_first_key(access->acc_attr_map);
	}
	else {
		grm_log_debug("grac_access_find_first_attr() : invalid argument");
	}
	return attr;
}

/**
  @brief   속성( 리소스) 목록 중에서 현재 처리중인 속성의  다음 속성을 구한다.
  @details
  		이 후 grac_access_find_next_attr()를 반복적으로 호출하여 모든 속성을 구할 수 있다.
  @param [in]  access	GracAccess 객체주소
  @return gchar*	속성명,  더 이상 없으면 NULL
 */
gchar* grac_access_find_next_attr(GracAccess *access)
{
	gchar *attr = NULL;

	if (access) {
		attr = (gchar*)grac_map_next_key(access->acc_attr_map);
	}
	else {
		grm_log_debug("grac_access_find_next_attr() : invalid argument");
	}
	return attr;
}

/**
  @brief   지정된 속성의 접근 권한을 구한다.
  @details
  		이 후 grac_access_find_next_attr()를 반복적으로 호출하여 모든 속성을 구할 수 있다.
  @param [in]  access	GracAccess 객체주소
  @param [in]  attr		속성 (리소스 종류등)
  @return gchar*	문자열로 표현된 접근권한,  없으면 NULL
 */
gchar* grac_access_get_attr(GracAccess *access, gchar* attr)
{
	gchar *perm = NULL;

	if (access && attr) {
		if (grac_access_find_attr(access, attr) == TRUE) {
			perm = (gchar*)grac_map_get_data(access->acc_attr_map, attr);
		}
	}
	else {
		grm_log_debug("grac_access_get_attr_perm() : invalid argument");
	}

	return perm;
}


static gboolean _grac_acc_set_attr_perm(GracAccess *access, gchar* attr, gchar *perm, gboolean adj)
{
	gboolean ret = FALSE;
	if (access && attr && perm) {
		if (grac_access_find_attr(access, attr) == TRUE) {
			ret = grac_map_set_data(access->acc_attr_map, attr, perm);
			if (adj)
				_grac_acc_set_allow_by_perm(access, attr, perm);
		}
	}
	else {
		grm_log_debug("grac_access_set_attr_perm() : invalid argument");
	}
	return ret;
}


/**
  @brief   지정된 속성의 접근 권한을 설정한다.
  @details
  		grac_access_put_attr()과는 달리 속성이 없는 경우 오류로 처리한다.
  @param [in]  access	GracAccess 객체주소
  @param [in]  attr		속성 (리소스 종류등)
  @param [in]  perm		권한 (접근허가등)
  @return gboolean 성공여부
 */
gboolean grac_access_set_attr(GracAccess *access, gchar* attr, gchar *perm)
{
	return _grac_acc_set_attr_perm(access, attr, perm, TRUE);
}

/**
  @brief   누락된 속성이 없는지 확인하고 누락된 것은 기본권한으로 추가한다.
  @param [in]  access	GracAccess 객체주소
  @return gboolean 성공여부
 */
gboolean grac_access_fill_attr(GracAccess *access)
{
	gboolean ret = FALSE;

	if (access) {
		ret = TRUE;
		char* attr = grac_access_attr_find_first_attr ();
		while (attr) {
			if (grac_access_find_attr(access, attr) == FALSE) {
				char* perm = grac_access_attr_find_first_perm(attr);
				ret &= _grac_acc_put_attr(access, attr, perm, TRUE);
			}
			attr = grac_access_attr_find_next_attr ();
		}
	}

	return ret;
}

/**
  @brief   모든 속성을 기본권한으로 설정한다.
  @details
  		누락된 속성이 있는 경우는 해당 속성을 추가한다. \n
		grac_access_fill_attr()은 누락된 속성만 기본권한으로 추가하는 반면 \n
		이 함수는 모든 속성을 대상으로 기본권한으로 설정한다.
  @param [in]  access	GracAccess 객체주소
  @return gboolean 성공여부
 */
gboolean grac_access_init_attr(GracAccess *access)
{
	gboolean ret = FALSE;

	if (access) {
		access->acc_printer = GRAC_ACCESS_DENY;
		access->acc_network = GRAC_ACCESS_DENY;
		access->acc_usb = GRAC_ACCESS_DENY;
		access->acc_capture = GRAC_ACCESS_DENY;
		access->acc_harddisk = GRAC_ACCESS_DENY;

		ret = TRUE;
		char* attr = grac_access_attr_find_first_attr ();
		while (attr) {
			char* perm = grac_access_attr_find_first_perm(attr);
			ret &= _grac_acc_put_attr(access, attr, perm, TRUE);
			attr = grac_access_attr_find_next_attr ();
		}

	}

	return ret;
}

// 구 버전과의 호환을 위해 사용
static gboolean _grac_acc_set_allow(GracAccess *access, int target, gboolean allow)
{
	gboolean ret = FALSE;
	int	set;

	if (allow == TRUE) {
		set = GRAC_ACCESS_ALLOW;
	}
	else {
		set = GRAC_ACCESS_DENY;
	}

	if (access) {
		switch (target) {
		case GRAC_ATTR_PRINTER :
			access->acc_printer = set;
			ret = TRUE;
			break;
		case GRAC_ATTR_NETWORK :
			access->acc_network = set;
			ret = TRUE;
			break;
		case GRAC_ATTR_SCREEN_CAPTURE :
			access->acc_capture = set;
			ret = TRUE;
			break;
		case GRAC_ATTR_USB :
			access->acc_usb = set;
			ret = TRUE;
			break;
		case GRAC_ATTR_HOME_DIR :
			access->acc_harddisk = set;
			ret = TRUE;
			break;
		}
	}

	return ret;
}

/**
  @brief   리소스로의 접근 허용 여부 설정한다.
  @param [in]  access	GracAccess 객체주소
  @param [in]  target	리소스종류
  @param [in]  allow		허용여부
  @return gboolean 성공 여부
  @warning
  		구 버전 호환을 위한 함수이며 향후 삭제 예정이다.
 */
gboolean grac_access_set_allow(GracAccess *access, int target, gboolean allow)
{
	gboolean ret = FALSE;

	ret = _grac_acc_set_allow(access, target, allow);
	if (ret == TRUE) {
		int pval;
		if (allow == TRUE)
			pval = GRAC_PERM_ALLOW;
		else
			pval = GRAC_PERM_DENY;
		char* attr = grac_access_attr_get_attr_name(target);
		char* perm = grac_access_attr_get_perm_name(target, pval);
		_grac_acc_put_attr(access, attr, perm, FALSE);
	}

	return ret;
}

/**
  @brief   리소스로의 접근 허용 여부를 확인한다.
  @param [in]  access	GracAccess 객체주소
  @param [in]  target	리소스종류
  @return gboolean 허용 여부
  @warning
  		구 버전 호환을 위한 함수이며 향후 삭제 예정이다.
 */
gboolean grac_access_get_allow(GracAccess *access, int target)
{
	gboolean ret = FALSE;

	if (access) {
		switch (target) {
		case GRAC_ATTR_PRINTER :
			if (access->acc_printer == GRAC_ACCESS_ALLOW)
				ret = TRUE;
			break;
		case GRAC_ATTR_NETWORK :
			if (access->acc_network == GRAC_ACCESS_ALLOW)
				ret = TRUE;
			break;
		case GRAC_ATTR_SCREEN_CAPTURE :
			if (access->acc_capture == GRAC_ACCESS_ALLOW)
				ret = TRUE;
			break;
		case GRAC_ATTR_USB :
			if (access->acc_usb == GRAC_ACCESS_ALLOW)
				ret = TRUE;
			break;
		case GRAC_ATTR_HOME_DIR :
			if (access->acc_harddisk == GRAC_ACCESS_ALLOW)
				ret = TRUE;
			break;
		}
	}

	return ret;
}

static void _grac_acc_set_allow_by_perm(GracAccess *access, gchar* attr, gchar *perm)
{
	int perm_val = grac_access_attr_get_perm_value(attr, perm);
	int attr_val = grac_access_attr_get_attr_value(attr);
	int	target = attr_val;
	gboolean allow = FALSE;

	switch (perm_val) {
	case GRAC_PERM_ALLOW :
		allow = TRUE;
		break;
	case GRAC_PERM_DENY :
	default :
		allow = FALSE;
		break;
	}

	_grac_acc_set_allow(access, target, allow);

}


/**
  @brief   일반 사용자를 위한 기본 접근 권한으로 설정한다.
  @param [out]  acc	GracAccess 객체주소
  @return gboolean 성공 여부
 */
gboolean grac_access_set_default_of_guest(GracAccess* acc)
{
	gboolean done = FALSE;

	if (acc) {
		done = grac_access_init_attr (acc);
		done &= grac_access_set_allow(acc, GRAC_ATTR_HOME_DIR, TRUE);
		done &= grac_access_set_allow(acc, GRAC_ATTR_NETWORK, TRUE);		// 2016.11.16
	}

	return done;
}

/**
  @brief   관리자를 위한 기본 접근 권한으로 설정한다.
  @param [out]  acc	GracAccess 객체주소
  @return gboolean 성공 여부
 */
gboolean grac_access_set_default_of_admin(GracAccess* acc)
{
	gboolean done = FALSE;

	if (acc) {
		done = grac_access_init_attr (acc);
		done &= grac_access_set_allow(acc, GRAC_ATTR_NETWORK, TRUE);
		done &= grac_access_set_allow(acc, GRAC_ATTR_PRINTER, TRUE);
		done &= grac_access_set_allow(acc, GRAC_ATTR_SCREEN_CAPTURE, TRUE);
		done &= grac_access_set_allow(acc, GRAC_ATTR_USB, TRUE);
		done &= grac_access_set_allow(acc, GRAC_ATTR_HOME_DIR, TRUE);
	}

	return done;
}

/**
  @brief   URL를 위한 기본 접근 권한으로 설정한다.
  @param [out]  acc	GracAccess 객체주소
  @return gboolean 성공 여부
 */
gboolean grac_access_set_default_of_url (GracAccess* acc)
{
	gboolean done = FALSE;

	if (acc) {
		done = grac_access_init_attr (acc);
		done &= grac_access_set_allow(acc, GRAC_ATTR_NETWORK, TRUE);
		done &= grac_access_set_allow(acc, GRAC_ATTR_HOME_DIR, TRUE);
	}

	return done;
}

/**
  @brief   접근 권한 정보가 올바르게 설정되어 있는지를 확인한다.
  @param [in]  acc 	GracAccess 객체주소
  @return gboolean 확인결과
 */
gboolean grac_access_validate_attr(GracAccess* acc)
{
	gboolean done = FALSE;

	if (acc) {
		gchar *attr;
		done = TRUE;

		attr = grac_access_find_first_attr(acc);
		while (attr) {
			int	value = grac_access_attr_get_attr_value(attr);
			gboolean res = FALSE;
			if (value <= 0) {  // invalid attr
				res = grac_access_del_attr (acc, attr);
			}

			if (res)
				attr = grac_access_find_first_attr(acc);
			else
				attr = grac_access_find_next_attr(acc);
		}
	}

	return done;

}

#if 0
/**
 @brief   객체 복사에 의한 GracAccess  객체 생성
 @details
      	객체 지향형 언어의 생성자에 해당한다.
 @param [in] acc 복사할 객체 주소
 @return GracAccess* 생성된 객체 주소
 */
GracAccess* grac_access_alloc_copy(GracAccess *acc)
{
	GracAccess* new_acc = NULL;
	gboolean res = TRUE;

	if (acc) {
		new_acc = grac_access_alloc();
		if (new_acc) {
			memcpy(new_acc, acc, sizeof(GracAccess));
			if (acc->acc_name) {
				new_acc->acc_name = strdup(acc->acc_name);
				if (new_acc->acc_name == NULL)
					res = FALSE;
			}
			if (acc->perm_str) {
				new_acc->perm_str = strdup(acc->perm_str);
				if (new_acc->perm_str == NULL)
					res = FALSE;
			}

			new_acc->acc_attr_map = grac_map_alloc();
			if (new_acc->acc_attr_map == NULL) {
				res = FALSE;
			}
			else {
				char *attr, *perm;
				attr = grac_access_find_first_attr(acc);
				while (attr) {
					perm = grac_access_get_attr(acc, attr);
					res &= grac_access_put_attr (new_acc, attr, perm);
					attr = grac_access_find_next_attr(acc);
				}
				res &= grac_access_fill_attr(new_acc);
			}

			if (res == FALSE) {
				grac_access_free(&new_acc);
				grm_log_error("can't copy grac_access");
			}

		}
	}

	return new_acc;
}
#endif
