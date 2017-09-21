/*
 * test_lsblk.c
 *
 *  Created on: 2017. 9. 15.
 *      Author: yang
 */


#include "cutility.h"
#include "sys_etc.h"

#include <json-c/json.h>
extern void json_dump_one(json_object *jobj, int depth);

gboolean check_device_json(json_object *jdevice, int depth)
{
	int		read_only = -1;
	int		removal = -1;

	int		dev_name_t = -1;
	int		dev_type_t = -1;
	int		mount_t = -1;

	char	*dev_name = NULL;
	char	*dev_type = NULL;
	char	*mount_point = NULL;
	gboolean root = FALSE;

	json_object_object_foreach(jdevice, key, val)
	{
		if (c_strmatch(key, "name")) {
			dev_name_t = 1;
			dev_name = (char*)json_object_get_string(val);
		}
		else if (c_strmatch(key, "rm")) {
			removal = json_object_get_int(val);
		}
		else if (c_strmatch(key, "ro")) {
			read_only = json_object_get_int(val);
		}
		else if (c_strmatch(key, "type")) {
			dev_type_t = 1;
			dev_type = (char*)json_object_get_string(val);
		}
		else if (c_strmatch(key, "mountpoint")) {
			json_type jtype = json_object_get_type(val);
			if (jtype == json_type_null) {
				mount_t = 0;
				mount_point = "";
			}
			else if (jtype == json_type_string) {
				mount_t = 1;
				mount_point = (char*)json_object_get_string(val);
				if (c_strmatch(mount_point, "/") ||
						c_strcmp(mount_point, "/boot", 5, -1) == 0)
				{
					root = TRUE;
				}
			}
			else {
				printf("invalid value\n");
			}
		}
	} // foreach

	printf("***** %d **** ro:%d rm:%d %d:%s %d:%s %d:%s\n", depth,
			read_only, removal, dev_name_t, dev_name, dev_type_t, dev_type, mount_t, mount_point );

	json_object *jchild;
	json_bool jres = json_object_object_get_ex(jdevice, "children", &jchild);
	if (jres) {
		json_type jtype = json_object_get_type(jchild);
		if (jtype == json_type_array) {
			int  count = json_object_array_length(jchild);
			int  idx;
			gboolean child_root = FALSE;
			for (idx=0; idx<count; idx++) {
				json_object* jentry = json_object_array_get_idx(jchild, idx);
				child_root |= check_device_json(jentry, depth+1);
			}
			printf("child root : %d\n", (int)child_root);
			root |= child_root;
		}
		else {
			printf("invalid type\n");
		}
		json_object_put(jchild);
	}

	printf("root : %d\n", (int)root);

	if (depth == 1 && root)
		printf ("<<<<<<< %s >>>>>\n", dev_name);

	return root;
}

void t_block_device_json()
{
	char	output[2048];
	gboolean res;

	res =  sys_run_cmd_get_output("/bin/lsblk -J", "test", output, sizeof(output));
	if (res == FALSE) {
		printf("error\n");
		return;
	}
	else {
		printf("%d\n", (int)c_strlen(output, 2048));
		printf(output);
	}

	enum json_tokener_error jerror;
	json_object *jroot;

	jroot = json_tokener_parse_verbose(output, &jerror);

	if (jerror != json_tokener_success) {
		printf("parser error : %s", json_tokener_error_desc(jerror));
		json_object_put(jroot);
		return;
	}

	json_type jtype;

//	jtype = json_object_get_type(jroot);
//	printf("jroot type : %s\n", json_type_to_name(jtype));

	//json_dump_one(jroot, 1);

	json_object *jblock;
	json_bool jres = json_object_object_get_ex(jroot, "blockdevices", &jblock);
	if (jres) {
		jtype = json_object_get_type(jblock);
		printf("type : %s\n", json_type_to_name(jtype));
		if (jtype == json_type_array) {
			int  count = json_object_array_length(jblock);
			int  idx;
			for (idx=0; idx<count; idx++) {
				json_object* jentry = json_object_array_get_idx(jblock, idx);
				check_device_json(jentry, 1);
			}
		}
		else {
			printf("invalid type\n");
		}
		json_dump_one(jblock, 1);
		json_object_put(jblock);
	}
	else {
		printf("Invalid \n");
		return;
	}

	json_object_put(jroot);

	printf("End \n");
}
