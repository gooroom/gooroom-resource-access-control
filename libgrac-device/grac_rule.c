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
#include "grac_rule_printer_cups.h"
#include "grac_printer.h"
#include "grac_blockdev.h"
#include "grac_map.h"
#include "grac_config.h"
#include "grac_log.h"
#include "cutility.h"
#include "sys_ipt.h"
#include "sys_user.h"
#include "sys_etc.h"

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

	sys_ipt *ipt;
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
			grac_log_debug("grac_rule_alloc() : out of memory");
			grac_rule_free(&rule);
			return NULL;
		}

		rule->ipt = sys_ipt_alloc();
		if (rule->ipt == NULL) {
			grac_log_debug("grac_rule_alloc() : out of memory");
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

		sys_ipt_alloc(&rule->ipt);
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

//static gboolean _validate_resource  		(char *resource);
//static gboolean _validate_permission		(char *permission);
//static gboolean _validate_net_addr  	  (char *addr, gboolean ipv6);
//static gboolean _validate_net_protocol	(char *protocol);
//static gboolean _validate_net_port    	(char *protocol, char *port);
//static gboolean _validate_net_direction	(char *direction);
//static gboolean _validate_mac_addr  	  (char *addr);

static gboolean _grac_rule_load_protocol_rule_json(GracRuleNetwork *rule_net, json_object *jproto)
{
	//char *func = (char*)__FUNCTION__;
	gboolean done = TRUE;

	if (rule_net == NULL || jproto == NULL)
		return FALSE;

	json_bool 	jres;
	json_type		jtype;

	jtype = json_object_get_type(jproto);
	if (jtype != json_type_object) {
		grac_log_error("grac_rule.c : invalid rule : need object type for protocol");
		return FALSE;
	}

	char	*protocol_name = NULL;
	json_object *jobj_proto_name;

	// pass 1 - get protocol
	jres = json_object_object_get_ex(jproto, "protocol", &jobj_proto_name);
	if (!jres) {
		grac_log_error("grac_rule.c : invalid rule : not defined protocol key");
		return FALSE;
	}
	jtype = json_object_get_type(jobj_proto_name);
	if (jtype == json_type_null) {
		grac_log_error("grac_rule.c : invalid rule : omitted protocol name");
		return FALSE;
	}
	else if (jtype == json_type_string) {
		protocol_name = (char*)json_object_get_string(jobj_proto_name);
		if (c_strlen(protocol_name, 256) <= 0) {
			grac_log_error("grac_rule.c : invalid rule : omitted protocol name");
			return FALSE;
		}
	}
	else {
		grac_log_error("grac_rule.c : invalid rule : need string type for protocol name");
		return FALSE;
	}

	int		protocol_idx;

	grac_rule_network_add_protocol(rule_net, protocol_name);
	protocol_idx = grac_rule_network_find_protocol(rule_net, protocol_name);
	if (protocol_idx < 0) {
		grac_log_error("grac_rule.c : can't add protocol information (%s)", protocol_name);
		return FALSE;
	}

	// pass 2 - collect ports
	done = TRUE;

	json_object_object_foreach(jproto, key, val)
	{
		if (c_strmatch(key, "protocol")) {
			;	// no operation
		}
		else if (c_strmatch(key, "src_port")) {
			jtype = json_object_get_type(val);
			if (jtype == json_type_array) {
				int  count = json_object_array_length(val);
				int  idx;
				for (idx=0; idx<count; idx++) {
					json_object* jentry = json_object_array_get_idx(val, idx);
					json_type jentry_type = json_object_get_type(jentry);
					gboolean add_res;
					if (jentry_type == json_type_null) {
						;	// skip
					}
					else if (jentry_type == json_type_string) {
						char *port_str = (char*)json_object_get_string(jentry);
						if (c_strlen(port_str, 256) > 0) {
							add_res = grac_rule_network_add_src_port(rule_net, protocol_idx, port_str);
							if (add_res == FALSE)
								grac_log_error("grac_rule.c : can't add port (%s:%s)", protocol_name, port_str);
							done &= add_res;
						}
					}
					else {
						grac_log_error("grac_rule.c : invalid rule : need string type for port");
					}
				}
			}
			else if (jtype == json_type_string) {
				char *port_str = (char*)json_object_get_string(val);
				if (c_strlen(port_str, 256) > 0) {
					gboolean add_res;
					add_res = grac_rule_network_add_src_port(rule_net, protocol_idx, port_str);
					if (add_res == FALSE)
						grac_log_error("grac_rule.c : can't add port (%s:%s)", protocol_name, port_str);
					done &= add_res;
				}
			}
			else if (jtype == json_type_null) {
				;	//  skip
			}
			else {
				grac_log_error("grac_rule.c : invalid rule : need string array type for src_port");
			}
		}
		else if (c_strmatch(key, "dst_port")) {
			jtype = json_object_get_type(val);
			if (jtype == json_type_array) {
				int  count = json_object_array_length(val);
				int  idx;
				for (idx=0; idx<count; idx++) {
					json_object* jentry = json_object_array_get_idx(val, idx);
					json_type jentry_type = json_object_get_type(jentry);
					gboolean add_res;
					if (jentry_type == json_type_null) {
						;	// skip
					}
					else if (jentry_type == json_type_string) {
						char *port_str = (char*)json_object_get_string(jentry);
						if (c_strlen(port_str, 256) > 0) {
							add_res = grac_rule_network_add_dst_port(rule_net, protocol_idx, port_str);
							if (add_res == FALSE)
								grac_log_error("grac_rule.c : can't add port (%s:%s)", protocol_name, port_str);
							done &= add_res;
						}
					}
					else {
						grac_log_error("grac_rule.c : invalid rule : need string type for port");
					}
				}
			}
			else if (jtype == json_type_string) {
				char *port_str = (char*)json_object_get_string(val);
				if (c_strlen(port_str, 256) > 0) {
					gboolean add_res;
					add_res = grac_rule_network_add_dst_port(rule_net, protocol_idx, port_str);
					if (add_res == FALSE)
						grac_log_error("grac_rule.c : can't add port (%s:%s)", protocol_name, port_str);
					done &= add_res;
				}
			}
			else if (jtype == json_type_null) {
				;	//  skip
			}
			else {
				grac_log_error("grac_rule.c : invalid rule : need string array type for src_port");
			}
		}
		else {
			grac_log_debug("grac_rule.c : invalid rule : unknown key [%s] for protocol", key);
		}
	} // foreach

	return done;
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
			grac_log_error("%s : can't alloc network array of rule", func);
			return FALSE;
		}
	}

	json_type jtype = json_object_get_type(jobj_rule);
	if (jtype != json_type_object) {
		grac_log_error("grac_rule.c : invalid format for network rules");
		return FALSE;
	}

	GracRuleNetwork	*net_rule = grac_rule_network_alloc();
	if (net_rule == NULL) {
		grac_log_error("%s : can't alloc GracRuleNetwork", func);
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
				grac_log_debug("grac_rule.c : unknown state for <network/rules> : %s", perm_str);
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
				grac_log_debug("grac_rule.c : unknown direction for <network/rules> : %s", dir_str);
			}
		}
		else if (c_strmatch(key, "ports")) {
			jtype = json_object_get_type(val);
			if (jtype == json_type_array) {
				int  count = json_object_array_length(val);
				int  idx;
				for (idx=0; idx<count; idx++) {
					json_object* jentry = json_object_array_get_idx(val, idx);
					done &= _grac_rule_load_protocol_rule_json(net_rule, jentry);
				}
			}
			else if (jtype == json_type_object) {
				done &= _grac_rule_load_protocol_rule_json(net_rule, val);
			}
			else {
				grac_log_debug("grac_rule.c : invalid format for <network/rules> : %s", key);
			}
		}
		else {
			grac_log_debug("grac_rule.c : unknown key [%s] in <network/rules>", key);
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
				grac_log_error("_grac_rule_load_network_json() : invalid json type for %s", key);
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
				grac_log_error("_grac_rule_load_network_json() : invalid json type for %s", key);
				done = FALSE;
			}
		}
		else {
			grac_log_error("_grac_rule_load_network_json() : invalid key : %s", key);
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
				grac_log_error("%s : can't alloc mac address array of rule", func);
				return FALSE;
			}
		}

		gchar *add_mac_addr = c_strdup(mac_addr, 256);
		if (add_mac_addr == NULL) {
			grac_log_error("%s : can't alloc mac address to add", func);
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
		grac_log_error("%s : invalid json type for bluetooth", func);
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
				grac_log_error("%s : invalid json type for bluetooth/%s", func, key);
				done = FALSE;
			}
		}
		else if (c_strmatch(key, "mac_address")) {
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
						grac_log_error("%s : invalid json type for bluetooth/%s", func, key);
						done = FALSE;
					}
				}
			}
			else {
				grac_log_error("%s : invalid json type for bluetooth/%s", func, key);
				done = FALSE;
			}
		}
		else {
			grac_log_debug("%s : invalid key for bluetooth : %s", func, key);
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
		grac_log_error("_grac_rule_load_simple_json() : invalid json type for %s", key);
		break;
	}

	return done;
}


static gboolean _grac_rule_load_json(GracRule *rule, gchar* path)
{
	gboolean done = TRUE;
	json_object *jroot = json_object_from_file(path);


	if (jroot == NULL) {
		grac_log_error("_grac_rule_load_json() : not found or inavlide json file");
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
void _grac_rule_network_dump (GracRuleNetwork *net_rule, int seq, FILE *out_fp)
{
	if (net_rule == NULL) {
		fprintf(out_fp, "%d: null data\n", seq);
		return;
	}

	int	get_res;
	gboolean allow;
	char* set_data;

	get_res = grac_rule_network_get_address(net_rule, &set_data);
	if (get_res < 0)
		fprintf(out_fp, "%d: ipaddress : <error>\n", seq);
	else	if (get_res == 0)
		fprintf(out_fp, "%d: ipaddress : <not set>\n", seq);
	else
		fprintf(out_fp, "%d: ipaddress : %s\n", seq, set_data);

	get_res = grac_rule_network_get_allow(net_rule, &allow);
	if (get_res < 0)
		fprintf(out_fp, "%d: state : <error>\n", seq);
	else	if (get_res == 0)
		fprintf(out_fp, "%d: state : <not set>\n", seq);
	else
		fprintf(out_fp, "%d: state : %d\n", seq, (int)allow);

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
		fprintf(out_fp, "%d: direction : <error>\n", seq);
	else	if (get_res == 0)
		fprintf(out_fp, "%d: direction : <not set>\n", seq);
	else
		fprintf(out_fp, "%d: direction : %s\n", seq, set_data);

	int cnt, ix;
	int	port_cnt, port_idx;

	cnt = grac_rule_network_protocol_count(net_rule);
	fprintf(out_fp, "%d: protocol count=%d\n", seq, cnt);
	for (ix = 0; ix<cnt; ix++) {
		get_res = grac_rule_network_get_protocol(net_rule, ix, &set_data);
		if (get_res < 0)
			fprintf(out_fp, "\t<error>");
		else	if (get_res == 0)
			fprintf(out_fp, "\t<not set>");
		else
			fprintf(out_fp, "\t[%s]", set_data);
		fprintf(out_fp, "\n");

		port_cnt = grac_rule_network_src_port_count(net_rule, ix);
		fprintf(out_fp, "\t\tsrc_port count=%d", port_cnt);
		for (port_idx = 0; port_idx<port_cnt; port_idx++) {
			get_res = grac_rule_network_get_src_port(net_rule, ix, port_idx, &set_data);
			if (get_res < 0)
				fprintf(out_fp, "  <error>");
			else	if (get_res == 0)
				fprintf(out_fp, "  <not set>");
			else
				fprintf(out_fp, "  [%s]", set_data);
		}
		fprintf(out_fp, "\n");

		port_cnt = grac_rule_network_dst_port_count(net_rule, ix);
		fprintf(out_fp, "\t\tdst_port count=%d", port_cnt);
		for (port_idx = 0; port_idx<port_cnt; port_idx++) {
			get_res = grac_rule_network_get_dst_port(net_rule, ix, port_idx, &set_data);
			if (get_res < 0)
				fprintf(out_fp, "  <error>");
			else	if (get_res == 0)
				fprintf(out_fp, "  <not set>");
			else
				fprintf(out_fp, "  [%s]", set_data);
		}
		fprintf(out_fp, "\n");
	}

	fprintf(out_fp, "\n");
}


// only to test
void grac_rule_dump (GracRule *rule, FILE *out_fp)
{
	if (rule == NULL)
		return;

	if (out_fp == NULL)
		out_fp = stdout;

	fprintf(out_fp, "<<<Network>>>\n");
	if (rule->network_count == 0) {
			fprintf(out_fp, "-- no data --\n");
	}
	else {
			int	idx;
			fprintf(out_fp, "count of network rules : %d\n", rule->network_count);
			for (idx=0; idx<rule->network_count; idx++) {
				GracRuleNetwork *net_rule = g_ptr_array_index (rule->network_array, idx);
				_grac_rule_network_dump (net_rule, idx, out_fp);
			}
	}
	fprintf(out_fp, "\n");

	const char *key, *data;
	key = grac_map_first_key(rule->res_perm_map);
	while (key) {
		data = grac_map_get_data(rule->res_perm_map, key);
		fprintf(out_fp, "<%s> = <%s>\n", key, data);
		key = grac_map_next_key(rule->res_perm_map);
	}
	fprintf(out_fp, "\n");

	fprintf(out_fp, "<<<MAC address for bluetooth>>>\n");
	if (rule->bluetooth_mac_count == 0) {
			fprintf(out_fp, "-- no data --\n");
	}
	else {
			int	idx;
			fprintf(out_fp, "count of macc adddress for bluetooth : %d\n", rule->bluetooth_mac_count);
			for (idx=0; idx < rule->bluetooth_mac_count; idx++) {
				char *mac = g_ptr_array_index (rule->bluetooth_mac_array, idx);
				fprintf(out_fp, "\t[%s]", mac);
			}
			fprintf(out_fp, "\n");
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
//		grac_rule_dump (data, NULL);

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
		grac_log_error("%s : can't get address", func);
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
		grac_log_error("%s : out of memory", func);
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
			grac_log_error("%s : can't get address info", func);
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
			grac_log_error("%s : can't get direction", func);
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
			grac_log_error("%s : can't get state", func);
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
		grac_log_error("%s : making ipt_rule : %s", func, log_addr);
	}
	done &= setB;

	if (done == FALSE) {
		sys_ipt_rule_free(&ipt_rule);
		return FALSE;
	}

	int	protocol_count = grac_rule_network_protocol_count(net_rule);

	// protocol && port
	if (protocol_count == 0) {	// ignore ports
		appB = sys_ipt_append_rule(rule->ipt, ipt_rule);
		if (appB == FALSE) {
			grac_log_error("%s : applying ipt_rule : %s, all protocols, all ports", func, log_addr);
		}
		done &= appB;
	}
	else {
		int protocol_idx;
		gchar *protocol_name;
		int	s_port_count;
		int	d_port_count;

		for (protocol_idx=0; protocol_idx<protocol_count; protocol_idx++) {
			s_port_count = grac_rule_network_src_port_count(net_rule, protocol_idx);
			d_port_count = grac_rule_network_dst_port_count(net_rule, protocol_idx);
			res = grac_rule_network_get_protocol(net_rule, protocol_idx, &protocol_name);
			if (res < 0) {
				grac_log_error("%s : can't get protocol", func);
				done = FALSE;
			}
			else if (res == 1) {
				setB = sys_ipt_rule_set_protocol_name(ipt_rule, protocol_name);
				if (setB == FALSE) {
					grac_log_error("%s : making ipt_rule : %s, %s", func, log_addr, protocol_name);
					continue;
				}
				sys_ipt_rule_clear_src_port(ipt_rule);
				sys_ipt_rule_clear_dst_port(ipt_rule);

				if (c_strimatch(protocol_name, "tcp") || c_strimatch(protocol_name, "udp"))
				 {
					int 	port_idx;
					char	*port_str;
					if (s_port_count + d_port_count == 0) {
						appB = sys_ipt_append_rule(rule->ipt, ipt_rule);
						if (appB == FALSE) {
							grac_log_error("%s : applying ipt_rule : %s, %s, no ports", func, log_addr, protocol_name);
						}
					}
					if (s_port_count > 0) {
						for (port_idx=0; port_idx < s_port_count; port_idx++) {
							res = grac_rule_network_get_src_port(net_rule, protocol_idx, port_idx, &port_str);
							if (res == 1)
								sys_ipt_rule_add_src_port_str(ipt_rule, port_str);
						}
						appB = sys_ipt_append_rule(rule->ipt, ipt_rule);
						if (appB == FALSE) {
							grac_log_error("%s : applying ipt_rule : %s, %s, with src_ports", func, log_addr, protocol_name);
						}
						done &= appB;

						sys_ipt_rule_clear_src_port(ipt_rule);
					}

					if (d_port_count > 0) {
						for (port_idx=0; port_idx < d_port_count; port_idx++) {
							res = grac_rule_network_get_dst_port(net_rule, protocol_idx, port_idx, &port_str);
							if (res == 1)
								sys_ipt_rule_add_dst_port_str(ipt_rule, port_str);
						}
						appB = sys_ipt_append_rule(rule->ipt, ipt_rule);
						if (appB == FALSE) {
							grac_log_error("%s : applying ipt_rule : %s, %s, with dst_ports", func, log_addr, protocol_name);
						}
						done &= appB;
					}
				}
				else {
					if (s_port_count + d_port_count > 0) {
						grac_log_error("%s : applying ipt_rule : %s, %s, not allowd port : ignore", func, log_addr, protocol_name);
					}
					appB = sys_ipt_append_rule(rule->ipt, ipt_rule);
					if (appB == FALSE) {
						grac_log_error("%s : applying ipt_rule : %s, %s, no ports", func, log_addr, protocol_name);
					}
					done &= appB;
				}
			}
		} // for (porotcol_idx)
	}

	sys_ipt_rule_free(&ipt_rule);

	return done;
}


static gboolean _grac_rule_apply_network(GracRule* rule)
{
	gboolean done = TRUE;
	int	perm_id;

	grac_log_debug("start : %s()", __FUNCTION__);

	sys_ipt_set_log(rule->ipt, TRUE, "GRAC: Disallowed Network ");		// Maximunm 29 chars
                                              /* 12345678901234567890123456789 */

	// no data
	if (rule == NULL) {
		// set deny
		done &= sys_ipt_clear_all(rule->ipt);
		done &= sys_ipt_set_policy(rule->ipt, FALSE);
//	done &= sys_ipt_insert_all_drop_rule();
		return done;
	}

	// set only default
	if (rule->network_count == 0) {
		done &= sys_ipt_clear_all(rule->ipt);
		perm_id = grac_rule_get_perm_id(rule, GRAC_RESOURCE_NETWORK);
		if (perm_id == GRAC_PERMISSION_ALLOW)
			done &= sys_ipt_set_policy(rule->ipt, TRUE);
		else
			done &= sys_ipt_set_policy(rule->ipt, FALSE);
		return done;
	}

	done &= sys_ipt_clear_all(rule->ipt);

	perm_id = grac_rule_get_perm_id(rule, GRAC_RESOURCE_NETWORK);
	if (perm_id == GRAC_PERMISSION_ALLOW)
		done &= sys_ipt_set_policy(rule->ipt, TRUE);
	else 	// deny or error - default permission is deny.
		done &= sys_ipt_set_policy(rule->ipt, FALSE);

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
		done &= sys_ipt_append_all_accept_rule(rule->ipt);
	else 	// deny or error - default permission is deny.
		done &= sys_ipt_append_all_drop_rule(rule->ipt);

	return done;
}

static gboolean _grac_rule_apply_udev_rule(GracRule *rule)
{
	gboolean done = FALSE;
	const char *map_path;
	char	tmp_file[1024];
	int		result;
	char	udev_rule_target[2048];
	int		uid;

	grac_log_debug("start : %s()", __FUNCTION__);

	uid = sys_user_get_login_uid();
	if (uid < 0) {
		grac_log_warning("%s(): can't get login name", __FUNCTION__);
		g_snprintf(udev_rule_target, sizeof(udev_rule_target), "/var/run/user/%s", grac_config_file_udev_rules());
		g_snprintf(tmp_file, sizeof(tmp_file), "/var/run/user/grac_tmp_udev");
	}
	else {
		g_snprintf(tmp_file, sizeof(tmp_file), "/var/run/user/%s", grac_config_file_udev_rules());
		unlink(tmp_file);
		g_snprintf(tmp_file, sizeof(tmp_file), "/var/run/user/grac_tmp_udev");
		unlink(tmp_file);

		g_snprintf(udev_rule_target, sizeof(udev_rule_target), "/var/run/user/%d/%s", uid, grac_config_file_udev_rules());
		g_snprintf(tmp_file, sizeof(tmp_file), "/var/run/user/%d/grac_tmp_udev", uid);
	}

/*
	c_strcpy(tmp_file, "/tmp/grac_tmp_XXXXXX", sizeof(tmp_file));
	res = mkstemp(tmp_file);
	if (res == -1) {
		const char *tmp_path;
		tmp_path = grac_config_dir_grac_data();
		g_snprintf(tmp_file, sizeof(tmp_file), "/%s/grac_tmp_udev", tmp_path);
	}
*/

	map_path = grac_config_path_udev_map_local();

	if (map_path) {
		gboolean made;
		GracRuleUdev *udev_rule;
		udev_rule = grac_rule_udev_alloc(map_path);
		if (udev_rule) {
			made = grac_rule_udev_make_rules(udev_rule, rule, tmp_file);
			if (made) {
				unlink(udev_rule_target);
				result = rename(tmp_file, udev_rule_target);
				if (result == -1) {
					done = FALSE;
					grac_log_error("%s() : rename error : %s --> %s : %s", __FUNCTION__, tmp_file, udev_rule_target, strerror(errno));
				}
			}
			else {
				grac_log_error("%s() : grac_rule_udev_make_rules", __FUNCTION__);
			}

			done = made;
		}
		grac_rule_udev_free(&udev_rule);
	}

	char *udev_path_link = (char*)grac_config_path_udev_rules();
	if (udev_path_link == NULL) {
		done = FALSE;
	}
	else {
		if (done == TRUE) {
			unlink(udev_path_link);
			result = symlink(udev_rule_target, udev_path_link);
			if (result != 0) {
				grac_log_error("%s(): can't make link : %s", __FUNCTION__, strerror(errno) );
				done = FALSE;
			}
		}
	}


	if (done) {
		gboolean res;
		char	*cmd;

		cmd = "udevadm control --reload";
		res = sys_run_cmd_no_output (cmd, "apply-rule");
		if (res == FALSE)
			grac_log_error("%s(): can't run %s", __FUNCTION__, cmd);
		done &= res;

		// no trigger for subsystem==PCI, triggering when setting rescan
		 {
			char *cmd = "echo 1 > /sys/bus/pci/rescan";
			res = sys_run_cmd_no_output (cmd, "apply-rule");
			if (res == FALSE)
				grac_log_error("%s(): can't run %s", __FUNCTION__, cmd);
			done &= res;
		}

		// no need for subsystem==USB, users should reinsert USB device
		// resinserting USB sets bAuthorized=1 automatically

		cmd = "udevadm trigger -s bluetooth -s net -s input -s video4linux -s pci -s sound -s hidraw -c add";
		res = sys_run_cmd_no_output (cmd, "apply-rule");
		if (res == FALSE)
			grac_log_error("%s(): can't run %s", __FUNCTION__, cmd);

		cmd = "udevadm trigger -s block -p ID_USB_DRIVER=usb-storage -c add";
		res = sys_run_cmd_no_output (cmd, "apply-rule");
		if (res == FALSE)
			grac_log_error("%s(): can't run %s", __FUNCTION__, cmd);
		done &= res;
	}

	return done;
}

static gboolean _grac_rule_apply_hook(GracRule *rule)
{
	gboolean done = FALSE;
	int	perm_id;

	grac_log_debug("start : %s()", __FUNCTION__);

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

// iptables -I OUTPUT -p tcp --dport 515 -j DROP
static gboolean _grac_rule_disallow_network_printer(GracRule *rule, int port)
{
	gboolean done = FALSE;

	if (rule == NULL)
		return FALSE;

	sys_ipt_rule *ipt_rule = sys_ipt_rule_alloc();

	if (ipt_rule) {
		done = TRUE;
		char	port_str[32];

		g_snprintf(port_str, sizeof(port_str), "%u", port);

		sys_ipt_set_log(rule->ipt, TRUE, "GRAC: Disallowed Printer ");		// Maximunm 29 chars
                                                  /* 12345678901234567890123456789 */
		done &= sys_ipt_rule_set_target  (ipt_rule, SYS_IPT_TARGET_DROP);
		done &= sys_ipt_rule_set_chain   (ipt_rule, SYS_IPT_CHAIN_B_OUTPUT);
		done &= sys_ipt_rule_set_protocol(ipt_rule, SYS_IPT_PROTOCOL_TCP);
		done &= sys_ipt_rule_add_dst_port_str(ipt_rule, port_str);

		done &= sys_ipt_insert_rule(rule->ipt, ipt_rule);

		sys_ipt_rule_free(&ipt_rule);
	}

	return done;
}

static gboolean _grac_rule_apply_cups_printer(GracRule* rule)
{
	gboolean done = TRUE;
	int			 perm_id;
	gboolean allow;

	grac_log_debug("start : %s()", __FUNCTION__);

	perm_id = grac_rule_get_perm_id(rule, GRAC_RESOURCE_PRINTER);
	if (perm_id == GRAC_PERMISSION_ALLOW)
		allow = TRUE;
	else 	// deny or error - default permission is deny.
		allow = FALSE;

	// 사용자 목록 초기화, 기타 초기 처리
	done = grac_rule_printer_cups_init();
	if (done == FALSE) {
		grac_log_error("%s() : Initialize error for CUPS", __FUNCTION__);
		return FALSE;
	}

	//
	// 사용자  등록없이 처리 (모든 사용자를 대상으로 한다)
	//

	// CUPS에 적용 요청
	done = grac_rule_printer_cups_apply(allow);
	if (done == FALSE) {
		grac_log_error("%s() : Apply error for CUPS", __FUNCTION__);
	}

	grac_rule_printer_cups_final();

	return done;
}


// 추가 설정
static gboolean _grac_rule_apply_extra(GracRule *rule)
{
	gboolean done = TRUE;
	int res_id, perm_id;

	grac_log_debug("start : %s()", __FUNCTION__);

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
		case GRAC_RESOURCE_PRINTER :
			if (perm_id == GRAC_PERMISSION_ALLOW)
				grac_printer_apply(TRUE);
			else
				grac_printer_apply(FALSE);
			if (perm_id == GRAC_PERMISSION_DISALLOW) {
				_grac_rule_disallow_network_printer(rule, grac_config_network_printer_port());
			}
			done &= _grac_rule_apply_cups_printer(rule);
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
			fprintf(out_fp, "count of macc adddress for bluetooth : %d\n", rule->bluetooth_mac_count);
			for (idx=0; idx < rule->bluetooth_mac_count; idx++) {
				char *mac = g_ptr_array_index (rule->bluetooth_mac_array, idx);
				fprintf(out_fp, "\t[%s]", mac);
			}
			fprintf(out_fp, "\n");
	}
*/

	return done;
}

gboolean grac_rule_apply (GracRule *rule)
{
	gboolean done = FALSE;

	grac_log_debug("start : %s()", __FUNCTION__);
	if (rule) {

		done = _grac_rule_apply_network(rule);

		// special process for USB
		int perm_id = grac_rule_get_perm_id(rule, GRAC_RESOURCE_USB);
		if (perm_id == GRAC_PERMISSION_READONLY)
			grac_blockdev_apply_readonly();
		else if (perm_id == GRAC_PERMISSION_READWRITE)
			grac_blockdev_apply_normal();
		else if (perm_id == GRAC_PERMISSION_ALLOW)
			grac_blockdev_apply_normal();
		else
			grac_blockdev_apply_disallow();

		done &= _grac_rule_apply_udev_rule(rule);

		done &= _grac_rule_apply_hook(rule);

		done &= _grac_rule_apply_extra(rule);
	}

	return done;
}

gboolean grac_rule_apply_allow_all(GracRule *rule)
{
	gboolean done = FALSE;

	if (rule) {
		// clear all filter
		done = sys_ipt_clear_all(rule->ipt);
		done &= sys_ipt_set_policy(rule->ipt, TRUE);

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
				done &= grac_rule_set_perm_by_name (rule, res_name, perm_name);
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
				done &= grac_rule_set_perm_by_name (rule, res_name, perm_name);
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
gboolean grac_rule_set_perm_by_name(GracRule *rule, gchar* res_name, gchar* perm_name)
{
	gboolean done = FALSE;

	if (rule && res_name && perm_name)
		done = grac_map_set_data(rule->res_perm_map, res_name, perm_name);

	return done;

}

gboolean grac_rule_set_perm_by_id  (GracRule *rule, int res_id, int perm_id)
{
	gboolean done = FALSE;

	if (rule && res_id >= 0 && perm_id >= 0) {
		char *res_name = grac_resource_get_resource_name(res_id);
		char *perm_name = grac_resource_get_permission_name(perm_id);
		if (res_name && perm_name)
			done = grac_rule_set_perm_by_name(rule, res_name, perm_name);
	}

	return done;
}

int		   grac_rule_bluetooth_mac_count(GracRule *rule)
{
	int	count = 0;

	if (rule) {
		count = rule->bluetooth_mac_count;
	}

	return count;
}

gboolean grac_rule_bluetooth_mac_get_addr(GracRule *rule, int idx, char *addr, int addr_size)
{
	gboolean done = FALSE;

	if (rule) {
		if (idx >= 0 && idx < rule->bluetooth_mac_count) {
			char *ptr = g_ptr_array_index (rule->bluetooth_mac_array, idx);
			if (c_strlen(ptr, addr_size+1) < addr_size-1) {
				c_strcpy(addr, ptr, addr_size);
				done = TRUE;
			}
		}
	}

	return done;
}

gboolean grac_rule_pre_process()
{
	gboolean done = TRUE;

	done &= grac_printer_init();
	done &= grac_blockdev_init();

	return done;
}

gboolean grac_rule_post_process()
{
	gboolean done = TRUE;

	done &= grac_printer_end();
	done &= grac_blockdev_end();

	return done;
}
