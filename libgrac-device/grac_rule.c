/*
 ============================================================================
 Name        : grac_rule.c
 Author      : 
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */

/**
  @file 	 	grac_rule.c
  @brief		리소스 접근 권한 정보
  @details	class GraciRule 정의 (GTK style의 c언어를 이용한 객체처리 사용) \n
  				헤더파일 :  	grac_rule.h	\n
  				라이브러리:	libgrac-device.so
 */

#include "grac_rule.h"
#include "grac_rule_network.h"
#include "grac_rule_udev.h"
#include "grac_rule_hook.h"
#include "grac_map.h"
#include "grac_config.h"
#include "grm_log.h"
#include "cutility.h"
#include "sys_ipt.h"

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


struct _GracRule
{
	// 매체,리소스별 접근 권한 정보
	// 주의  1. network도 포함 - network 전체를 제어한다.
	//    2. bluetooth도 포함
	GracMap *res_perm_map;	// {char* resource, char *permission}

	// 네트워크 주소별 접근제어
	GPtrArray *network_array;			// GracRuleNetwork*
	int	network_count;

	// special process
	// 블루투스  MAC list
	GPtrArray *bluetooth_mac_array;			// char*
	int	bluetooth_mac_count;
};


/**
 @brief   GracRule  객체 생성
 @details
      	객체 지향형 언어의 생성자에 해당한다.
 @return GracRule* 생성된 객체 주소
 */
GracRule* grac_rule_alloc()
{
	GracRule *rule = malloc(sizeof(GracRule));
	if (rule) {
		memset(rule, 0, sizeof(GracRule));

		rule->res_perm_map = grac_map_alloc();
		if (rule->res_perm_map == NULL) {
			grm_log_debug("grac_rule_alloc() : out of memory");
			grac_rule_free(&rule);
			return NULL;
		}
	}

	return rule;
}

/**
 @brief   GracRule 객체 해제
 @details
      객체 지향형 언어의 소멸자에  해당한다. 단 자동으로 불려지지 않고 필요한 시점에서 직접 호출하여야 한다.\n
      해제된  객체 주소값으로 0으로 설정하기 위해 이중 포인터를 사용한다.
 @param [in,out]  pAcc  해제하고자 하는 객체 주소를 담는 변수의 주소 (double pointer)
 */
void grac_rule_free(GracRule **pRule)
{
	if (pRule != NULL && *pRule != NULL) {
		GracRule *rule = *pRule;

		grac_rule_clear(rule);

		grac_map_free(&rule->res_perm_map);

		free(rule);

		*pRule = NULL;
	}
}

void grac_rule_clear(GracRule *rule)
{
	if (rule) {

		grac_map_clear(rule->res_perm_map);

		if (rule->network_array) {
			int	i;
			for (i=0; i < rule->network_count; i++) {
				GracRuleNetwork *net_rule = g_ptr_array_index (rule->network_array, i);
				grac_rule_network_free(&net_rule);
			}
			g_ptr_array_free(rule->network_array, TRUE);
			rule->network_count = 0;
			rule->network_array = NULL;
		}

		if (rule->bluetooth_mac_array) {
			int i;
			for (i=0; i < rule->bluetooth_mac_count; i++) {
				char *mac_addr = g_ptr_array_index (rule->bluetooth_mac_array, i);
				c_free(&mac_addr);
			}
			g_ptr_array_free(rule->bluetooth_mac_array, TRUE);
			rule->bluetooth_mac_count = 0;
			rule->bluetooth_mac_array = NULL;
		}

	}
}


static gboolean _grac_rule_load_network_rule_json(GracRule *rule, json_object *jobj_rule)
{
	char *func = "_grac_rule_load_network_rule_json()";
	gboolean done = TRUE;

	if (rule == NULL || jobj_rule == NULL)
		return FALSE;

	if (rule->network_array == NULL) {
		rule->network_array = g_ptr_array_new();
		if (rule->network_array == NULL) {
			grm_log_error("%s : can't alloc network array of rule", func);
			return FALSE;
		}
	}

	json_type jtype = json_object_get_type(jobj_rule);
	if (jtype != json_type_object) {
		grm_log_error("grac_rule.c : invalid format for network rules");
		return FALSE;
	}

	GracRuleNetwork	*net_rule = grac_rule_network_alloc();
	if (net_rule == NULL) {
		grm_log_error("%s : can't alloc GracRuleNetwork", func);
		return FALSE;
	}

	json_object_object_foreach(jobj_rule, key, val) {
		if (c_strmatch(key, "ipaddress")) {
			char *addr_str = (char*)json_object_get_string(val);
			done &= grac_rule_network_set_address(net_rule, addr_str);
		}
		else if (c_strmatch(key, "state")) {
			char *perm_str = (char*)json_object_get_string(val);
			if (c_strimatch(perm_str, "allow")) {
				grac_rule_network_set_allow(net_rule, TRUE);
			}
			else if (c_strimatch(perm_str, "disallow")) {
				grac_rule_network_set_allow(net_rule, FALSE);
			}
			else {
				grm_log_debug("grac_rule.c : unknown state for <network/rules> : %s", perm_str);
			}
		}
		else if (c_strmatch(key, "direction")) {
			char *dir_str = (char*)json_object_get_string(val);
			if (c_strimatch(dir_str, "all")) {
				grac_rule_network_set_direction(net_rule, GRAC_NETWORK_DIR_ALL);
			}
			else if (c_strimatch(dir_str, "inbound")) {
				grac_rule_network_set_direction(net_rule, GRAC_NETWORK_DIR_INBOUND);
			}
			else if (c_strimatch(dir_str, "outbound")) {
				grac_rule_network_set_direction(net_rule, GRAC_NETWORK_DIR_OUTBOUND);
			}
			else {
				grm_log_debug("grac_rule.c : unknown direction for <network/rules> : %s", dir_str);
			}
		}
		else if (c_strmatch(key, "protocol")) {
			char *proto_str;
			jtype = json_object_get_type(val);
			if (jtype == json_type_array) {
				int  count = json_object_array_length(val);
				int  idx;
				for (idx=0; idx<count; idx++) {
					json_object* jentry = json_object_array_get_idx(val, idx);
					proto_str = (char*)json_object_get_string(jentry);
					done &= grac_rule_network_add_protocol(net_rule, proto_str);
				}
			}
			else if (jtype == json_type_string) {
				proto_str = (char*)json_object_get_string(val);
				done &= grac_rule_network_add_protocol(net_rule, proto_str);
			}
			else {
				grm_log_debug("grac_rule.c : invalid format for <network/rules> : %s", key);
			}
		}
		else if (c_strmatch(key, "src-port")) {
			char *port_str;
			jtype = json_object_get_type(val);
			if (jtype == json_type_array) {
				int  count = json_object_array_length(val);
				int  idx;
				for (idx=0; idx<count; idx++) {
					json_object* jentry = json_object_array_get_idx(val, idx);
					port_str = (char*)json_object_get_string(jentry);
					done &= grac_rule_network_add_src_port(net_rule, port_str);
				}
			}
			else if (jtype == json_type_string) {
				port_str = (char*)json_object_get_string(val);
				done &= grac_rule_network_add_src_port(net_rule, port_str);
			}
			else {
				grm_log_debug("grac_rule.c : invalid format for <network/rules> : %s", key);
			}
		}
		else if (c_strmatch(key, "dst-port")) {
			char *port_str;
			jtype = json_object_get_type(val);
			if (jtype == json_type_array) {
				int  count = json_object_array_length(val);
				int  idx;
				for (idx=0; idx<count; idx++) {
					json_object* jentry = json_object_array_get_idx(val, idx);
					port_str = (char*)json_object_get_string(jentry);
					done &= grac_rule_network_add_dst_port(net_rule, port_str);
				}
			}
			else if (jtype == json_type_string) {
				port_str = (char*)json_object_get_string(val);
				done &= grac_rule_network_add_dst_port(net_rule, port_str);
			}
			else {
				grm_log_debug("grac_rule.c : invalid format for <network/rules> : %s", key);
			}
		}
		else {
			grm_log_debug("grac_rule.c : unknown key [%s] in <network/rules>", key);
		}
	} // foreach

	if (done) {
		g_ptr_array_add(rule->network_array, net_rule);
		rule->network_count++;
	}
	else {
		grac_rule_network_free(&net_rule);
	}

	return done;
}

static gboolean _grac_rule_load_network_json(GracRule *rule, json_object *jobj_net)
{
	gboolean done = TRUE;

	if (rule == NULL || jobj_net == NULL)
		return FALSE;

	json_object_object_foreach(jobj_net, key, val) {
		if (c_strmatch(key, "state")) {  // basic permission of network
			json_type jtype = json_object_get_type(val);
			if (jtype == json_type_string) {
				const char *res_perm = json_object_get_string(val);
				char *res_key = grac_resource_get_resource_name(GRAC_RESOURCE_NETWORK);
				done = grac_map_set_data(rule->res_perm_map, res_key, res_perm);
			}
			else {
				grm_log_error("_grac_rule_load_network_json() : invalid json type for %s", key);
				done = FALSE;
			}
		}
		else if (c_strmatch(key, "rules")) {
			json_type jtype = json_object_get_type(val);
			if (jtype == json_type_object) {	// single rule
				done &= _grac_rule_load_network_rule_json(rule, val);
			}
			else if (jtype == json_type_array) { // multiple rules
				int  count = json_object_array_length(val);
				int  idx;
				for (idx=0; idx<count; idx++) {
					json_object* jentry = json_object_array_get_idx(val, idx);
					done &= _grac_rule_load_network_rule_json(rule, jentry);
				}
			}
			else {
				grm_log_error("_grac_rule_load_network_json() : invalid json type for %s", key);
				done = FALSE;
			}
		}
		else {
			grm_log_error("_grac_rule_load_network_json() : invalid key : %s", key);
		}
	}

	return done;
}

gboolean _grac_rule_add_bluetooth_mac(GracRule *rule, char *mac_addr)
{
	char *func = "_grac_rule_add_bluetooth_mac()";
	gboolean done = FALSE;

	if (rule && mac_addr) {

		if (rule->bluetooth_mac_array == NULL) {
			rule->bluetooth_mac_array = g_ptr_array_new();
			if (rule->bluetooth_mac_array == NULL) {
				grm_log_error("%s : can't alloc mac address array of rule", func);
				return FALSE;
			}
		}

		gchar *add_mac_addr = c_strdup(mac_addr, 256);
		if (add_mac_addr == NULL) {
			grm_log_error("%s : can't alloc mac address to add", func);
		}
		else {
			g_ptr_array_add(rule->bluetooth_mac_array, add_mac_addr);
			rule->bluetooth_mac_count++;
			done = TRUE;
		}
	}

	return done;

}

gboolean _grac_rule_load_bluetooth_json(GracRule *rule, json_object *jobj)
{
	char *func = "_grac_rule_load_bluetooth_json()";
	gboolean done = FALSE;
	json_type jtype;

	if (rule == NULL || jobj == NULL)
		return FALSE;

	jtype = json_object_get_type(jobj);
	if (jtype != json_type_object) {
		grm_log_error("%s : invalid json type for bluetooth", func);
		return FALSE;
	}

	done = TRUE;
	json_object_object_foreach(jobj, key, val) {
		if (c_strmatch(key, "state")) {
			jtype = json_object_get_type(val);
			if (jtype == json_type_string) {
				char *res_perm = (char*)json_object_get_string(val);
				char *res_key = grac_resource_get_resource_name(GRAC_RESOURCE_BLUETOOTH);
				done &= grac_map_set_data(rule->res_perm_map, res_key, res_perm);
			}
			else {
				grm_log_error("%s : invalid json type for bluetooth/%s", func, key);
				done = FALSE;
			}
		}
		else if (c_strmatch(key, "mac-address")) {
			jtype = json_object_get_type(val);
			if (jtype == json_type_string) {
				char *mac = (char*)json_object_get_string(val);
				done &= _grac_rule_add_bluetooth_mac(rule, mac);
			}
			else if (jtype == json_type_array) {
				int  count = json_object_array_length(val);
				int  idx;
				for (idx=0; idx<count; idx++) {
					json_object* jentry = json_object_array_get_idx(val, idx);
					if (json_object_get_type(jentry) == json_type_string) {
						char *mac = (char*)json_object_get_string(jentry);
						done &= _grac_rule_add_bluetooth_mac(rule, mac);
					}
					else {
						grm_log_error("%s : invalid json type for bluetooth/%s", func, key);
						done = FALSE;
					}
				}
			}
			else {
				grm_log_error("%s : invalid json type for bluetooth/%s", func, key);
				done = FALSE;
			}
		}
		else {
			grm_log_debug("%s : invalid key for bluetooth : %s", func, key);
		}
	}
	return done;
}

gboolean _grac_rule_load_simple_json(GracRule *rule, char *key, json_object *jobj)
{
	gboolean done = FALSE;
	json_type jtype;
	const char *perm;

	if (rule == NULL || key == NULL || jobj == NULL)
		return FALSE;

	jtype = json_object_get_type(jobj);
	switch (jtype) {
	case json_type_string :
		perm = json_object_get_string(jobj);
		done = grac_map_set_data(rule->res_perm_map, key, perm);
		break;
	case json_type_array :
	case json_type_object :
	default:
		grm_log_error("_grac_rule_load_simple_json() : invalid json type for %s", key);
		break;
	}

	return done;
}


static gboolean _grac_rule_load_json(GracRule *rule, gchar* path)
{
	gboolean done = TRUE;
	json_object *jroot = json_object_from_file(path);


	if (jroot == NULL) {
		grm_log_error("_grac_rule_load_json() : not found or inavlide json file");
		return FALSE;
	}

	grac_rule_clear(rule);

	//json_object *jobj_network;
	//json_bool res = json_object_object_get_ex(jroot, "network", &jobj_network);

	json_object_object_foreach(jroot, key, val) {
		if (c_strmatch(key, "network")) {
			done &= _grac_rule_load_network_json(rule, val);
		}
		else if (c_strmatch(key, "bluetooth")) {
			done &= _grac_rule_load_bluetooth_json(rule, val);
		}
		else {
			done &= _grac_rule_load_simple_json(rule, key, val);
		}

	} // foreach

	json_object_put(jroot);

	return done;
}

// only to test
void _grac_rule_network_dump (GracRuleNetwork *net_rule, int seq)
{
	if (net_rule == NULL) {
		printf("%d: null data\n", seq);
		return;
	}

	int	get_res;
	gboolean allow;
	char* set_data;

	get_res = grac_rule_network_get_address(net_rule, &set_data);
	if (get_res < 0)
		printf("%d: ipaddress : <error>\n", seq);
	else	if (get_res == 0)
		printf("%d: ipaddress : <not set>\n", seq);
	else
		printf("%d: ipaddress : %s\n", seq, set_data);

	get_res = grac_rule_network_get_allow(net_rule, &allow);
	if (get_res < 0)
		printf("%d: state : <error>\n", seq);
	else	if (get_res == 0)
		printf("%d: state : <not set>\n", seq);
	else
		printf("%d: state : %d\n", seq, (int)allow);

	grac_network_dir_t	dir = 0;
	get_res = grac_rule_network_get_direction(net_rule, &dir);
	if (dir == GRAC_NETWORK_DIR_INBOUND)
		set_data = "INBOUND";
	else if (dir == GRAC_NETWORK_DIR_OUTBOUND)
		set_data = "OUTBOUND";
	else if (dir == GRAC_NETWORK_DIR_ALL)
		set_data = "ALL";
	else
		set_data = "unknown??";
	if (get_res < 0)
		printf("%d: direction : <error>\n", seq);
	else	if (get_res == 0)
		printf("%d: direction : <not set>\n", seq);
	else
		printf("%d: direction : %s\n", seq, set_data);

	int cnt, ix;

	cnt = grac_rule_network_protocol_count(net_rule);
	printf("%d: protocol count=%d", seq, cnt);
	for (ix = 0; ix<cnt; ix++) {
		get_res = grac_rule_network_get_protocol(net_rule, ix, &set_data);
		if (get_res < 0)
			printf("\t<error>");
		else	if (get_res == 0)
			printf("\t<not set>");
		else
			printf("\t[%s]", set_data);
	}
	printf("\n");

	cnt = grac_rule_network_src_port_count(net_rule);
	printf("%d: src_port count=%d", seq, cnt);
	for (ix = 0; ix<cnt; ix++) {
		get_res = grac_rule_network_get_src_port(net_rule, ix, &set_data);
		if (get_res < 0)
			printf("  <error>");
		else	if (get_res == 0)
			printf("  <not set>");
		else
			printf("  [%s]", set_data);
	}
	printf("\n");

	cnt = grac_rule_network_dst_port_count(net_rule);
	printf("%d: dst_port count=%d", seq, cnt);
	for (ix = 0; ix<cnt; ix++) {
		get_res = grac_rule_network_get_dst_port(net_rule, ix, &set_data);
		if (get_res < 0)
			printf("  <error>");
		else	if (get_res == 0)
			printf("  <not set>");
		else
			printf("  [%s]", set_data);
	}
	printf("\n");
}


// only to test
void _grac_rule_dump (GracRule *rule)
{
	if (rule == NULL)
		return;

	printf("<<<Network>>>\n");
	if (rule->network_count == 0) {
			printf("-- no data --\n");
	}
	else {
			int	idx;
			printf("count of network rules : %d\n", rule->network_count);
			for (idx=0; idx<rule->network_count; idx++) {
				GracRuleNetwork *net_rule = g_ptr_array_index (rule->network_array, idx);
				_grac_rule_network_dump (net_rule, idx);
			}
	}
	printf("\n");

	const char *key, *data;
	key = grac_map_first_key(rule->res_perm_map);
	while (key) {
		data = grac_map_get_data(rule->res_perm_map, key);
		printf("<%s> = <%s>\n", key, data);
		key = grac_map_next_key(rule->res_perm_map);
	}
	printf("\n");

	printf("<<<MAC address for bluetooth>>>\n");
	if (rule->bluetooth_mac_count == 0) {
			printf("-- no data --\n");
	}
	else {
			int	idx;
			printf("count of macc adddress for bluetooth : %d\n", rule->bluetooth_mac_count);
			for (idx=0; idx < rule->bluetooth_mac_count; idx++) {
				char *mac = g_ptr_array_index (rule->bluetooth_mac_array, idx);
				printf("\t[%s]", mac);
			}
			printf("\n");
	}
}


gboolean grac_rule_load (GracRule *data, gchar* path)
{
	gboolean done = FALSE;

	if (data) {
		if (path == NULL)
			path = (char*)grac_config_path_grac_rules(NULL);

		if (path) {
			done = _grac_rule_load_json(data, path);
		}
	}

//	if (done)
//		_grac_rule_dump (data);

	return done;
}

static gboolean _grac_network_apply_one(GracRule *rule, GracRuleNetwork *net_rule)
{
	char *func = "_grac_network_apply_one()";
	gboolean done = TRUE;
	gboolean setB = TRUE;
	gboolean appB;
	sys_ipt_rule 	*ipt_rule;
	char *log_addr;
	char  *ipaddr;
	int	res;

	if (rule == NULL || net_rule == NULL)
		return FALSE;

	res = grac_rule_network_get_address(net_rule, &ipaddr);
	if (res < 0) {
		grm_log_error("%s : can't get address", func);
		return FALSE;
	}

	if (res == 0) {
		log_addr = "All Addresses";
		ipaddr = NULL;
	}
	else {
		log_addr = ipaddr;
	}

	ipt_rule = sys_ipt_rule_alloc();
	if (ipt_rule == NULL) {
		grm_log_error("%s : out of memory", func);
		return FALSE;
	}

	sys_ipt_rule_init(ipt_rule);

	// address
	if (ipaddr != NULL) {
		char *addr;
		int	 mask;
		gboolean ipv6;

		res = grac_rule_network_get_address_ex(net_rule, &addr, &mask, &ipv6);
		if (res < 0) {
			grm_log_error("%s : can't get address info", func);
			done = FALSE;
		}
		else if (res == 1) {
			if (ipv6)
				setB &= sys_ipt_rule_set_addr6_str(ipt_rule, addr);
			else
				setB &= sys_ipt_rule_set_addr_str (ipt_rule, addr);

			if (mask > 0)
				setB &= sys_ipt_rule_set_mask (ipt_rule, mask);
		}
	}

	// mac address
	//{
	//	char *mac;
	//	res = grac_rule_network_get_mac(net_rule, &mac);
	//	setB &= sys_ipt_rule_set_mac_addr_str(ipt_rule, mac);
	//}

	// direction
	{
		grac_network_dir_t dir;
		res = grac_rule_network_get_direction(net_rule, &dir);
		if (res < 0) {
			grm_log_error("%s : can't get direction", func);
			done = FALSE;
		}
		else if (res == 1){
			if (dir == GRAC_NETWORK_DIR_ALL)
				setB &= sys_ipt_rule_set_chain (ipt_rule, SYS_IPT_CHAIN_B_INOUT);
			else if (dir == GRAC_NETWORK_DIR_INBOUND)
				setB &= sys_ipt_rule_set_chain (ipt_rule, SYS_IPT_CHAIN_B_INPUT);
			else if (dir == GRAC_NETWORK_DIR_OUTBOUND)
				setB &= sys_ipt_rule_set_chain (ipt_rule, SYS_IPT_CHAIN_B_OUTPUT);
			else
				setB &= sys_ipt_rule_set_chain (ipt_rule, SYS_IPT_CHAIN_B_INOUT);
		}
		else {
			setB &= sys_ipt_rule_set_chain (ipt_rule, SYS_IPT_CHAIN_B_INOUT);
		}
	}

	// permission (state)
	{
		gboolean allow;
		res = grac_rule_network_get_allow(net_rule, &allow);
		if (res < 0) {
			grm_log_error("%s : can't get state", func);
			done = FALSE;
		}
		else if (res == 1) {
			if (allow)
				setB &= sys_ipt_rule_set_target (ipt_rule, SYS_IPT_TARGET_ACCEPT);
			else
				setB &= sys_ipt_rule_set_target (ipt_rule, SYS_IPT_TARGET_DROP);
		}
		else {
			setB &= sys_ipt_rule_set_target (ipt_rule, SYS_IPT_TARGET_DROP);
		}
	}

	if (setB == FALSE) {
		grm_log_error("%s : making ipt_rule : %s", func, log_addr);
	}
	done &= setB;

	if (done == FALSE) {
		sys_ipt_rule_free(&ipt_rule);
		return FALSE;
	}

	int	protocol_count = grac_rule_network_protocol_count(net_rule);
	int	s_port_count = grac_rule_network_src_port_count(net_rule);
	int	d_port_count = grac_rule_network_dst_port_count(net_rule);

	// protocol && port
	if (protocol_count == 0) {	// ignore ports
		if (s_port_count + d_port_count > 0) {
			grm_log_debug("%s : ignore port info because of no protocol", func);
		}

		appB = sys_ipt_append_rule(ipt_rule);
		if (appB == FALSE) {
			grm_log_error("%s : applying ipt_rule : %s, all protocols, all ports", func, log_addr);
		}
		done &= appB;
	}
	else {
		int protocol_idx;
		for (protocol_idx=0; protocol_idx<protocol_count; protocol_idx++) {
			gchar *protocol_name;
			res = grac_rule_network_get_protocol(net_rule, protocol_idx, &protocol_name);
			if (res < 0) {
				grm_log_error("%s : can't get protocol", func);
				done = FALSE;
			}
			else if (res == 1) {
				setB = sys_ipt_rule_set_protocol_name(ipt_rule, protocol_name);
				if (setB == FALSE) {
					grm_log_error("%s : making ipt_rule : %s, %s", func, log_addr, protocol_name);
					continue;
				}
				sys_ipt_rule_clear_src_port(ipt_rule);
				sys_ipt_rule_clear_dst_port(ipt_rule);

				if (s_port_count + d_port_count > 0 &&
				   ( c_strimatch(protocol_name, "tcp") || c_strimatch(protocol_name, "udp")) )
				 {
					int 	port_idx;
					char	*port_str;

					if (s_port_count > 0) {
						for (port_idx=0; port_idx < s_port_count; port_idx++) {
							res = grac_rule_network_get_src_port(net_rule, port_idx, &port_str);
							if (res == 1)
								sys_ipt_rule_add_src_port_str(ipt_rule, port_str);
						}
						appB = sys_ipt_append_rule(ipt_rule);
						if (appB == FALSE) {
							grm_log_error("%s : applying ipt_rule : %s, %s, all src_ports", func, log_addr, protocol_name);
						}
						done &= appB;

						sys_ipt_rule_clear_src_port(ipt_rule);
					}

					if (d_port_count > 0) {
						for (port_idx=0; port_idx < d_port_count; port_idx++) {
							res = grac_rule_network_get_dst_port(net_rule, port_idx, &port_str);
							if (res == 1)
								sys_ipt_rule_add_dst_port_str(ipt_rule, port_str);
						}
						appB = sys_ipt_append_rule(ipt_rule);
						if (appB == FALSE) {
							grm_log_error("%s : applying ipt_rule : %s, %s, all dst_ports", func, log_addr, protocol_name);
						}
						done &= appB;
					}
				}
				else {
					appB = sys_ipt_append_rule(ipt_rule);
					if (appB == FALSE) {
						grm_log_error("%s : applying ipt_rule : %s, %s, all ports", func, log_addr, protocol_name);
					}
					done &= appB;
				}
			}
		}
	}

	sys_ipt_rule_free(&ipt_rule);

	return done;
}


static gboolean _grac_rule_apply_network(GracRule* rule)
{
	gboolean done = TRUE;
	int	perm_id;

	sys_ipt_set_log(TRUE, "*** Gooroom Grac ***");

	// no data
	if (rule == NULL || rule->network_count == 0) {
		// set deny
		done &= sys_ipt_clear_all();
		done &= sys_ipt_set_policy(FALSE);
		done &= sys_ipt_insert_all_drop_rule();
		return done;
	}

	done &= sys_ipt_clear_all();

	perm_id = grac_rule_get_perm_id(rule, GRAC_RESOURCE_NETWORK);
	if (perm_id == GRAC_PERMISSION_ALLOW)
		done &= sys_ipt_set_policy(TRUE);
	else 	// deny or error - default permission is deny.
		done &= sys_ipt_set_policy(FALSE);

	int	idx;
	for (idx=0; idx < rule->network_count; idx++) {
		GracRuleNetwork *net_rule;
		net_rule = g_ptr_array_index (rule->network_array, idx);
		if (net_rule == NULL)
			continue;

		done &= _grac_network_apply_one(rule, net_rule);
	}

	perm_id = grac_rule_get_perm_id(rule, GRAC_RESOURCE_NETWORK);
	if (perm_id == GRAC_PERMISSION_ALLOW)
		done &= sys_ipt_append_all_accept_rule();
	else 	// deny or error - default permission is deny.
		done &= sys_ipt_append_all_drop_rule();

	return done;
}

static gboolean _grac_rule_apply_udev_rule(GracRule *rule)
{
	gboolean done = FALSE;
	const char *map_path;
	const char *udev_path;
	char	tmp_file[1024];
	int		res;

	c_strcpy(tmp_file, "/tmp/grac_tmp_XXXXXX", sizeof(tmp_file));
	res = mkstemp(tmp_file);
	if (res == -1) {
		const char *tmp_path;
		tmp_path = grac_config_dir_grac_data();
		snprintf(tmp_file, sizeof(tmp_file), "/%s/grac_tmp_udev", tmp_path);
	}

	map_path = grac_config_path_udev_map();
	udev_path = grac_config_path_udev_rules();

	if (map_path && udev_path) {
		gboolean made;
		GracRuleUdev *udev_rule;
		udev_rule = grac_rule_udev_alloc(map_path);
		if (udev_rule) {
			made = grac_rule_udev_make_rules(udev_rule, rule, tmp_file);
			if (made) {
				int n;
				unlink(udev_path);
				n = rename(tmp_file, udev_path);
				if (n == -1) {
					done = FALSE;
					grm_log_error("%s() : rename error : tmp --> rule", __FUNCTION__);
				}
			}
			else {
				grm_log_error("%s() : grac_rule_udev_make_rules", __FUNCTION__);
			}

			done = made;
		}
		grac_rule_udev_free(&udev_rule);
	}

	return done;
}

static gboolean _grac_rule_apply_hook(GracRule *rule)
{
	gboolean done = FALSE;
	int	perm_id;

	done = grac_rule_hook_init();

	if (done) {
		perm_id = grac_rule_get_perm_id(rule, GRAC_RESOURCE_CLIPBOARD);
		if (perm_id == GRAC_PERMISSION_ALLOW)
			done &= grac_rule_hook_allow_clipboard(TRUE);
		else
			done &= grac_rule_hook_allow_clipboard(FALSE);

		perm_id = grac_rule_get_perm_id(rule, GRAC_RESOURCE_S_CAPTURE);
		if (perm_id == GRAC_PERMISSION_ALLOW)
			done &= grac_rule_hook_allow_screenshooter(TRUE);
		else
			done &= grac_rule_hook_allow_screenshooter(FALSE);
	}

//if (done == FALSE)
//	done = grac_rule_hook_clear();

	return done;
}

// 추가 설정
static gboolean _grac_rule_apply_extra(GracRule *rule)
{
	gboolean done = TRUE;

	int res_id, perm_id;

	res_id = grac_resource_first_control_res_id();
	while (res_id >= 0) {
		perm_id = grac_rule_get_perm_id(rule, res_id);
		switch (res_id) {
		case GRAC_RESOURCE_NETWORK :	// already done by _grac_network_apply();
			break;
		case GRAC_RESOURCE_CLIPBOARD :
		case GRAC_RESOURCE_S_CAPTURE :
			break;
		case GRAC_RESOURCE_BLUETOOTH :
			if (perm_id >= 0) {
				; // udev rule + MAC
			}
			break;
		case GRAC_RESOURCE_OTHERS :	// ignore
		case -1:										// error
			break;

		default:
			break;
		}
		res_id = grac_resource_next_control_res_id();
	}

/*
	if (rule->bluetooth_mac_count == 0) {
			int	idx;
			printf("count of macc adddress for bluetooth : %d\n", rule->bluetooth_mac_count);
			for (idx=0; idx < rule->bluetooth_mac_count; idx++) {
				char *mac = g_ptr_array_index (rule->bluetooth_mac_array, idx);
				printf("\t[%s]", mac);
			}
			printf("\n");
	}
*/

	return done;
}

gboolean grac_rule_apply (GracRule *rule)
{
	gboolean done = FALSE;

	grm_log_debug("start : %s()", __FUNCTION__);
	if (rule) {
		done = _grac_rule_apply_network(rule);

		done &= _grac_rule_apply_udev_rule(rule);

		done &= _grac_rule_apply_hook(rule);

		done &= _grac_rule_apply_extra(rule);
	}

	return done;
}

gboolean grac_rule_apply_allow_all(GracRule *rule)
{
	gboolean done;

	grm_log_debug("start : %s()", __FUNCTION__);

	// clear all filter
	done = sys_ipt_clear_all();
	done &= sys_ipt_set_policy(TRUE);

	// clear udev rule
	const char *path;
	path = grac_config_path_udev_rules();
	if (path) {
		GracRuleUdev *udev_rule;
		udev_rule = grac_rule_udev_alloc(NULL);
		if (udev_rule)
			done &= grac_rule_udev_make_empty(udev_rule, path);
		grac_rule_udev_free(&udev_rule);
	}

	return done;
}


// if error, return -1
int grac_rule_get_perm_id(GracRule *rule, int resource)
{
	int	perm_id = -1;

	if (rule) {
		char *res_str = grac_resource_get_resource_name(resource);
		if (res_str) {
			char *perm_str = (char*)grac_map_get_data(rule->res_perm_map, res_str);
			perm_id = grac_resource_get_permission_id(perm_str);
		}
	}

	return perm_id;
}

/**
  @brief   일반 사용자를 위한 기본 접근 권한으로 설정한다.
  @param [out]  acc	GracRule 객체주소
  @return gboolean 성공 여부
 */
gboolean grac_rule_set_default_of_guest(GracRule* rule)
{
	gboolean done = FALSE;

	if (rule) {
		done = TRUE;

		char *res_name, *perm_name;
		int	 res_id, perm_id;
		grac_rule_clear (rule);

		res_name = grac_resource_find_first_resource();
		while (res_name) {
			res_id = grac_resource_get_resource_id(res_name);
			perm_id = -1;

			switch (res_id) {
			case GRAC_RESOURCE_NETWORK :
			case GRAC_RESOURCE_KEYBOARD :
			case GRAC_RESOURCE_MOUSE :
				perm_id = GRAC_PERMISSION_ALLOW;
				break;

			case GRAC_RESOURCE_USB_MEMORY:
			case GRAC_RESOURCE_CD_DVD :
				perm_id = GRAC_PERMISSION_READONLY;
				break;

			case GRAC_RESOURCE_USB :
			case GRAC_RESOURCE_OTHERS :		// ignore
			case -1:
				break;

			default:
				perm_id = GRAC_PERMISSION_DISALLOW;
				break;
			}

			perm_name = grac_resource_get_permission_name(perm_id);
			if (perm_name) {
				done &= grac_rule_set_res_perm_by_name (rule, res_name, perm_name);
			}

			res_name = grac_resource_find_next_resource();
		}
	}

	return done;
}

/**
  @brief   관리자를 위한 기본 접근 권한으로 설정한다.
  @param [out]  acc	GracRule 객체주소
  @return gboolean 성공 여부
 */
gboolean grac_rule_set_default_of_admin(GracRule* rule)
{
	gboolean done = FALSE;

	if (rule) {
		done = TRUE;

		char *res_name, *perm_name;
		int	 res_id, perm_id;
		grac_rule_clear (rule);

		res_name = grac_resource_find_first_resource();
		while (res_name) {
			res_id = grac_resource_get_resource_id(res_name);
			perm_id = -1;

			switch (res_id) {
			case GRAC_RESOURCE_USB_MEMORY:
			case GRAC_RESOURCE_CD_DVD :
				perm_id = GRAC_PERMISSION_READWRITE;
				break;

			case GRAC_RESOURCE_OTHERS :		// ignore
			case -1:
				break;

			default:
				perm_id = GRAC_PERMISSION_ALLOW;
				break;
			}

			perm_name = grac_resource_get_permission_name(perm_id);
			if (perm_name) {
				done &= grac_rule_set_res_perm_by_name (rule, res_name, perm_name);
			}

			res_name = grac_resource_find_next_resource();
		}
	}

	return done;
}

/**
  @brief   지정된 리소스의 접근 권한을 설정한다.
  @details
  		리소스가 이미 있는 경우 권한을 변경하고 리소스가 없는 경우 추가한다.
  @param [in]  rule	GracRule 객체주소
  @param [in]  res_name		리소스
  @param [in]  perm_name	권한 (접근허가등)
  @return gboolean 성공여부
 */
gboolean grac_rule_set_res_perm_by_name(GracRule *rule, gchar* res_name, gchar* perm_name)
{
	gboolean done = FALSE;

	if (rule && res_name && perm_name)
		done = grac_map_set_data(rule->res_perm_map, res_name, perm_name);

	return done;

}

gboolean grac_rule_set_res_perm_by_id  (GracRule *rule, int res_id, int perm_id)
{
	gboolean done = FALSE;

	if (rule && res_id >= 0 && perm_id >= 0) {
		char *res_name = grac_resource_get_resource_name(res_id);
		char *perm_name = grac_resource_get_permission_name(perm_id);
		done = grac_rule_set_res_perm_by_name(rule, res_name, perm_name);
	}

	return done;
}


#if 0
// 구버전 대응 - 매체제오방식 확정후 삭제 검토
gboolean grac_rule_apply_by_user (GracRule *grac_rule, gchar* username)
{
	gboolean done = TRUE;

	if (grac_rule) {
		done = grac_apply_access_by_user(grac_rule, username);
	}

	return done;
}


/**
  @brief   지정된 속성( 리소스)에 대한 접근 권한을 삭제한다.
  @details
  		이미 존재하는 경우는 권한을 변경한다.
  @param [in]  access	GracRule 객체주소
  @param [in]  attr		속성 (리소스 종류등)
  @return gboolean 성공여부
 */
gboolean grac_rule_del_attr(GracRule *access, gchar* attr)
{
	gboolean ret = FALSE;

	if (access && attr) {
		grac_map_del_data(access->acc_attr_map, attr);
		ret = TRUE;
	}
	else {
		grm_log_debug("grac_rule_del_attr() : invalid argument");
	}
	return ret;
}

/**
  @brief   지정된 속성( 리소스)이 존재하는지 확인한다.
  @param [in]  access	GracRule 객체주소
  @param [in]  attr		속성 (리소스 종류등)
  @return gboolean 존재 여부
 */
gboolean grac_rule_find_attr(GracRule *access, gchar* attr)
{
	gboolean ret = FALSE;
	if (access && attr) {
		if (grac_map_get_data(access->acc_attr_map, attr) != NULL)
			ret = TRUE;
	}
	else {
		grm_log_debug("grac_rule_find_attr() : invalid argument");
	}
	return ret;
}

/**
  @brief   속성( 리소스) 목록중 최초의 것을 구한다.
  @details
  		이 후 grac_rule_find_next_attr()를 반복적으로 호출하여 모든 속성을 구할 수 있다.
  @param [in]  access	GracRule 객체주소
  @return gchar*	속성명,  없으면 NULL
 */
gchar* grac_rule_find_first_attr(GracRule *access)
{
	gchar *attr = NULL;

	if (access) {
		attr = (gchar*)grac_map_first_key(access->acc_attr_map);
	}
	else {
		grm_log_debug("grac_rule_find_first_attr() : invalid argument");
	}
	return attr;
}

/**
  @brief   속성( 리소스) 목록 중에서 현재 처리중인 속성의  다음 속성을 구한다.
  @details
  		이 후 grac_rule_find_next_attr()를 반복적으로 호출하여 모든 속성을 구할 수 있다.
  @param [in]  access	GracRule 객체주소
  @return gchar*	속성명,  더 이상 없으면 NULL
 */
gchar* grac_rule_find_next_attr(GracRule *access)
{
	gchar *attr = NULL;

	if (access) {
		attr = (gchar*)grac_map_next_key(access->acc_attr_map);
	}
	else {
		grm_log_debug("grac_rule_find_next_attr() : invalid argument");
	}
	return attr;
}

/**
  @brief   지정된 속성의 접근 권한을 구한다.
  @details
  		이 후 grac_rule_find_next_attr()를 반복적으로 호출하여 모든 속성을 구할 수 있다.
  @param [in]  access	GracRule 객체주소
  @param [in]  attr		속성 (리소스 종류등)
  @return gchar*	문자열로 표현된 접근권한,  없으면 NULL
 */
gchar* grac_rule_get_attr(GracRule *access, gchar* attr)
{
	gchar *perm = NULL;

	if (access && attr) {
		if (grac_rule_find_attr(access, attr) == TRUE) {
			perm = (gchar*)grac_map_get_data(access->acc_attr_map, attr);
		}
	}
	else {
		grm_log_debug("grac_rule_get_attr_perm() : invalid argument");
	}

	return perm;
}


#endif
