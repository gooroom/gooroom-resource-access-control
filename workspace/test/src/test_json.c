/*
 * test_json.c
 *
 *  Created on: 2017. 6. 27.
 *      Author: yang
 */


#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>

#include <json-c/json.h>

//json_object_is_type
//json_object* json_object_from_file(const char *filename);

void json_dump_one(json_object *jobj, int depth)
{
	char tab[256];

	if (depth > 255)
		depth = 255;
	memset(tab, '-', depth);
	tab[depth] = 0;

	printf("%s%s\n", tab, json_object_to_json_string(jobj));

	json_type jtype = json_object_get_type(jobj);
	switch (jtype) {
	case json_type_null :
	case json_type_boolean :
	case json_type_double :
	case json_type_int :
	case json_type_string :
		printf("Not allowable at this location\n");
		break;
	case json_type_array :
		{
				int count = json_object_array_length(jobj);
				int idx;
				for (idx=0; idx<count; idx++) {
					json_object* jentry = json_object_array_get_idx(jobj, idx);
					printf("%s\t(%d) %s\n", tab, idx, json_object_to_json_string(jentry));
					json_type jtype_entry = json_object_get_type(jentry);
					if (jtype_entry == json_type_object || jtype_entry == json_type_array) {
						json_dump_one(jentry, depth+1);
					}
				}
		}
	break;
	case json_type_object :
		{
			json_object_object_foreach(jobj, key, val) {
				json_type jtype_val = json_object_get_type(val);
				printf("%s\tkey:%s val:%s --> %s\n", tab, key, json_object_to_json_string(val),json_type_to_name(jtype_val));
				if (jtype_val == json_type_object || jtype_val == json_type_array) {
					json_dump_one(val, depth+1);
				}
			} // foreach
		}
		break;
	default:
		printf("Invalid type\n");
	}

}

gboolean json_dump(char *data)
{
	printf("Data ----\n%s\n", data);

	enum json_tokener_error jerror;
	json_object *jroot;

	jroot = json_tokener_parse_verbose(data, &jerror);

	if (jerror != json_tokener_success) {
		printf("parser error : %s", json_tokener_error_desc(jerror));
		json_object_put(jroot);
		return FALSE;
	}

	json_type jtype = json_object_get_type(jroot);
	printf("type : %s\n", json_type_to_name(jtype));

	json_dump_one(jroot, 1);

	//json_object_is_type
	//json_object* json_object_from_file(const char *filename);


	json_object_put(jroot);
	return TRUE;
}

void test_json()
{
	char *data;

	data = "{ \"test\" : 1 }";
	json_dump(data);

	data = "{\"data1\" : \"aaa\", \"data2\" : \"bbb\"}";
	json_dump(data);

	data = "{ \"Test\" : {\"data1\" : \"aaa\", \"data2\" : \"bbb\"}}";
	json_dump(data);

	data = "{ \"Test\" : {\"data1\" : \"aaa\", \"data2\" : [\"bbb1\", \"bbb2\", { \"bbb-obj\" : 999 }, [111, 222, 333]]}}";
	json_dump(data);
}
