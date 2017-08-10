/*
 ============================================================================
 Name        : grac_url_list.c
 Author      : 
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */

#include "grac_config.h"
#include "grac_url.h"
#include "grac_url_list.h"
#include "grm_log.h"
#include "cutility.h"

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

struct _GracUrlList
{
	GPtrArray *data;			// GracUrl
	gint	data_count;

	gboolean black_list;

};

typedef struct _GracUrlList GracUrlList;

static gboolean _grac_url_list_add_url(GracUrlList *list, GracUrl* url);
static gboolean _grac_url_list_add_pattern(GracUrlList *list, gchar* pattern);
//static gboolean _grac_url_list_del_by_idx(GracUrlList *list, int idx);

static gboolean _grac_url_list_add_url(GracUrlList *list, GracUrl* url)
{
	gboolean done = FALSE;

	if (list == NULL || url == NULL)
		return FALSE;

	if (list->data) {
		g_ptr_array_add(list->data, url);
		list->data_count++;

		done = TRUE;
	}

	return done;
}

static gboolean _grac_url_list_add_pattern(GracUrlList *list, gchar* pattern)
{
	gboolean done = FALSE;
	GracUrl *url;

	if (list == NULL || c_strlen(pattern, 256) == 0)
		return FALSE;

	url = grac_url_alloc();
	if (url) {
		done = grac_url_set_pattern(url, pattern);
		if (done)
			done = _grac_url_list_add_url(list, url);
		if (done == FALSE)
			grac_url_free(&url);
	}

	return done;
}

/*
static gboolean _grac_url_list_del_by_idx(GracUrlList *list, int idx)
{
	gboolean done = FALSE;

	if (list && list->data) {
		if (idx >= 0 && idx < list->data_count) {
			GracUrl *url;
			url = g_ptr_array_index (list->data, idx);
			if (url != NULL) {
				grac_url_free(&url);
			}
			g_ptr_array_remove_index (list->data, idx);

			list->data_count--;

			done = TRUE;
		}
	}

	return done;
}
*/


GracUrlList* grac_url_list_alloc()
{
	GracUrlList* list = malloc(sizeof(GracUrlList));
	if (list) {
		memset(list, 0, sizeof(GracUrlList));
		list->data = g_ptr_array_new();
		if (list->data == NULL) {
			free(list);
			return NULL;
		}

		list->black_list = FALSE;		// default : white list
	}

	return list;
}

void grac_url_list_free(GracUrlList **plist)
{
	if (plist != NULL && *plist != NULL) {
		GracUrlList *list = *plist;
		int idx;
		for (idx=0; idx<list->data_count; idx++) {
			GracUrl *url;
			url = g_ptr_array_index (list->data, idx);
			if (url != NULL) {
				grac_url_free(&url);
			}
		}
		if (list->data != NULL)
			g_ptr_array_free(list->data, TRUE);

		free(list);

		*plist = NULL;
	}
}

void grac_url_list_clear(GracUrlList *list)
{
	if (list != NULL) {
		int idx;
		for (idx=0; idx<list->data_count; idx++) {
			GracUrl *url;
			url = g_ptr_array_index (list->data, 0);
			if (url != NULL)
				grac_url_free(&url);
			g_ptr_array_remove_index (list->data, 0);
		}
		list->data_count = 0;
	}
}

GracUrlList* grac_url_list_alloc_copy(GracUrlList *src)
{
	GracUrlList *new_list = NULL;

	if (src) {
		new_list = grac_url_list_alloc();
	}

	if (new_list) {

		new_list->black_list = src->black_list;

		int	 idx, count;
		count = grac_url_list_get_count(src);
		for (idx=0; idx<count; idx++) {
			GracUrl *url;
			url = g_ptr_array_index (src->data, idx);
			if (url != NULL)
				_grac_url_list_add_url(new_list, url);
		}
	}

	return new_list;
}

void	grac_url_list_set_blacklist(GracUrlList *list)
{
	if (list)
		list->black_list = TRUE;
}

void	grac_url_list_set_whitelist(GracUrlList *list)
{
	if (list)
		list->black_list = FALSE;
}

// 주의 : list 자체가 없을 때는  모두 불가 처리를 위해  list가 white인것처럼 처리
gboolean grac_url_list_is_blacklist(GracUrlList *list)
{
	gboolean res = TRUE;

	if (list == NULL) {
		res = FALSE;
	}
	else {
		res = list->black_list;
	}

	return res;
}

gboolean grac_url_list_is_allowed(GracUrlList *list, gchar *url_str)
{
	int	idx = -1;

	if (list && url_str) {
		idx = grac_url_list_find_match(list, url_str);
	}

	grm_log_debug("URL match result : %s -> %d", url_str, idx);

	gboolean blacklist;
	gboolean allow;

	blacklist = grac_url_list_is_blacklist(list);
	if (blacklist) {
		if (idx < 0)
			allow = TRUE;
		else
			allow = FALSE;
	}
	else {
		if (idx >= 0)
			allow = TRUE;
		else
			allow = FALSE;
	}

	grm_log_debug("allowed check : %s -> %d", url_str, (int)allow);

	return allow;
}

int	grac_url_list_get_count(GracUrlList *list)
{
	int count = 0;

	if (list)
		count = list->data_count;

	return count;
}

GracUrl *grac_url_list_get_url(GracUrlList *list, int idx)
{
	GracUrl *url = NULL;

	if (list && list->data) {
		if (idx >= 0 && idx < list->data_count) {
			url = g_ptr_array_index (list->data, idx);
		}
	}

	return url;
}

int	grac_url_list_find_match(GracUrlList *list, gchar *url_str)
{
	int idx = -1;
	int  i;
	GracUrl *url;

	if (list && url_str) {
		for (i=0; i<list->data_count; i++) {
			url = g_ptr_array_index (list->data, i);
			if (url) {
				if (grac_url_is_match(url, url_str)) {
					grm_log_debug("found-----------------");
					idx = i;
					break;
				}
			}
		}
	}

	return idx;
}

static gboolean _grac_url_list_load_json(GracUrlList *list, gchar* path)
{
	gboolean done = FALSE;
	json_object *jroot = json_object_from_file(path);

	if (jroot == NULL)
		return FALSE;

	json_object *jobj;
	json_bool res = json_object_object_get_ex(jroot, "url_list", &jobj);
	if (res) {
		int count = json_object_array_length(jobj);
		int idx;
		for (idx=0; idx<count; idx++) {
			json_object* jentry = json_object_array_get_idx(jobj, idx);
			_grac_url_list_add_pattern(list, (char*)json_object_get_string(jentry));
		}

		done = TRUE;
	}

	json_object_put(jroot);

	return done;
}

static gboolean _grac_url_list_save_json(GracUrlList *list, gchar* path)
{
	return FALSE;
}


// if path is NULL, path is default
gboolean grac_url_list_load (GracUrlList *list, char *path, gboolean blacklist)
{
	gboolean done = FALSE;

	if (list) {
		if (path == NULL)
			path = (char*)grac_config_path_url_list();

		if (path) {
			done = _grac_url_list_load_json(list, path);
			if (done) {
				if (blacklist)
					grac_url_list_set_blacklist(list);
				else
					grac_url_list_set_whitelist(list);
			}

			int i, cnt;
			GracUrl *url;
			cnt = grac_url_list_get_count(list);
			grm_log_debug("URL count : %d from JSON ", cnt);
			for (i=0; i<cnt; i++) {
				url = grac_url_list_get_url(list, i);
				char*pattern = grac_url_get_pattern(url);
				if (pattern)
					grm_log_debug("URL list [%d] : %s from JSON ", i, pattern);
				else
					grm_log_debug("URL list [%d] : invalid from JSON ", i);
			}
		}
	}

	return done;

}

gboolean grac_url_list_save (GracUrlList *list, char *path)
{
	gboolean done = FALSE;

	if (list) {
		if (path == NULL)
			path = (char*)grac_config_path_url_list();

		if (path) {
			done = _grac_url_list_save_json(list, path);
		}
	}

	return done;

}

