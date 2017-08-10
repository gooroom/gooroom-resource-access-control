/*
 ============================================================================
 Name        : gac_access_list.c
 Author      : 
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */


#include "gac_config.h"

#include "gac_access_list.h"

#include "grm_log.h"
#include "cutility.h"
#include "sys_user.h"
#include "sys_user_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include <json-c/json.h>
#include <json-c/json_util.h>


struct _GacAccessList
{
	char	*user_name;

	GacAccess *access;	// 접근권한

	GacAccess *admin_default_access;
	GacAccess *guest_default_access;

};

GacAccessList* gac_access_list_alloc()
{
	GacAccessList* data = malloc(sizeof(GacAccessList));
	if (data) {
		memset(data, 0, sizeof(GacAccessList));

		data->access = gac_access_alloc();
		data->admin_default_access = gac_access_alloc();
		data->guest_default_access = gac_access_alloc();
		if (data->access == NULL ||
				data->admin_default_access == NULL ||
				data->guest_default_access == NULL)
		{
			grm_log_error("gac_access_list_alloc() : out of memory");
			gac_access_list_free(&data);
		}
		else {
			gac_access_set_default_of_guest(data->guest_default_access);
			gac_access_set_default_of_admin(data->admin_default_access);
		}
	}

	return data;
}

void gac_access_list_free(GacAccessList **pdata)
{
	if (pdata != NULL && *pdata != NULL) {
		GacAccessList *data = *pdata;

		gac_access_free(&data->access);
		gac_access_free(&data->admin_default_access);
		gac_access_free(&data->guest_default_access);

		c_free(&data->user_name);

		free(data);

		*pdata = NULL;
	}
}

void gac_access_list_clear(GacAccessList *data)
{
	if (data != NULL) {

		gac_access_clear(data->access);

	}
}

GacAccessList* gac_access_list_alloc_copy(GacAccessList *src_data)
{
	GacAccessList *new_data = NULL;
	gboolean done = TRUE;

	if (src_data) {
		new_data = gac_access_list_alloc();
	}

	if (new_data) {
			new_data->access = gac_access_alloc_copy(src_data->access);
			if (new_data->access == NULL)
				done = FALSE;
			new_data->user_name = c_strdup(src_data->user_name, NAME_MAX);
	}

	if (done == FALSE) {
		gac_access_list_free(&new_data);
	}

	return new_data;
}


static gboolean _gac_access_list_load_json(GacAccessList *data, gchar* path)
{
	gboolean done = FALSE;
	json_object *jroot = json_object_from_file(path);

	if (jroot == NULL)
		return FALSE;

	json_object *jobj;
	json_bool res = json_object_object_get_ex(jroot, "simple_format", &jobj);
	if (res) {
		gac_access_clear(data->access);

		json_object_object_foreach(jobj, key, val) {
			//json_type jtype_val = json_object_get_type(val);
			gac_access_put_attr(data->access, key, (char*)json_object_get_string(val));
		} // foreach

		// json_object_put(jobj);  no need : json_object_object_get_ex();

		done = TRUE;
	}

	json_object_put(jroot);

	return done;
}

static gboolean _gac_access_list_save_json(GacAccessList *data, gchar* path)
{
	return FALSE;
}


gboolean gac_access_list_load (GacAccessList *data, gchar* path)
{
	gboolean done = FALSE;

	if (data) {
		if (path == NULL)
			path = (char*)gac_config_path_access();

		if (path) {
			done = _gac_access_list_load_json(data, path);
		}
	}

	return done;
}

gboolean gac_access_list_save (GacAccessList *data, gchar *path)
{
	gboolean done = FALSE;

	if (data) {
		if (path == NULL)
			path = (char*)gac_config_path_access();

		if (path) {
			done = _gac_access_list_save_json(data, path);
		}
	}

	return done;
}

GacAccess* gac_access_list_default_access_of_guest(GacAccessList* data)
{
	GacAccess *acc = NULL;

	if (data) {
		acc = data->guest_default_access;
	}

	return acc;
}

GacAccess* gac_access_list_default_access_of_admin(GacAccessList* data)
{
	GacAccess *acc = NULL;

	if (data) {
		acc = data->admin_default_access;
	}

	return acc;
}

gboolean gac_access_list_put_access_value(GacAccessList *data, gchar *key, gchar *value)
{
	gboolean done = FALSE;
	if (data)
		done = gac_access_put_attr(data->access, key, value);
	return done;
}

gboolean gac_access_list_set_access_value(GacAccessList *data, gchar *key, gchar *value)
{
	gboolean done = FALSE;
	if (data)
		done = gac_access_set_attr(data->access, key, value);
	return done;
}

gchar*   gac_access_list_get_access_value(GacAccessList *data, gchar *key)
{
	gchar *value = NULL;
	if (data)
		value = gac_access_get_attr(data->access, key);
	return value;
}

GacAccess* gac_access_list_get_access(GacAccessList *data)
{
	GacAccess* access = NULL;
	if (data)
		access = data->access;

	return access;
}
