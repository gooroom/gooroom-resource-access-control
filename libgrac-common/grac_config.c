/*
 * grac_config.c
 *
 *  Created on: 2016. 6. 13.
 *      Author: yang
 */

/**
  @file 	 	grac_config.c
  @brief		구름 권한 관리 시스템의 환경 설정 관리 함수
  @details	현재는 설정값 대부분이 프로그램 내에서 고정이 되어 있다. \n
         		일부는  /etc/goorom/grac.conf 를 이용하여 정의한다.
  				헤더파일 :  	grac_config.h	\n
  				라이브러리:	libgrac.so
  @warning
  		객체지향적으로  구현된 함수와 일반 함수의 혼용 정리 필요
 */

#include "grac_config.h"
#include "grm_log.h"
#include "sys_user.h"
#include "sys_etc.h"
#include "cutility.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

#include <glib.h>
#include <glib/gstdio.h>

struct _GracConfig {
	gint  max_path;
	gint	max_username;

	gchar	*dir_grac_data;

	gchar	*dir_grac_rule;

	gchar	*path_default_grac_rule;
	gchar	*path_user_grac_rule;

	gchar	*path_grac_udev_map_org;
	gchar	*path_grac_udev_map_local;

	gchar	*dir_udev_rules;
	gchar	*path_udev_rules;
	gchar	*file_udev_rules;

	gchar	*path_hook_screenshooter_so;
	gchar	*path_hook_screenshooter_conf;
	gchar	*path_hook_clipboard_so;
	gchar	*path_hook_clipboard_conf;

	gchar	*file_ld_so_preload;
	gchar	*path_ld_so_preload;
	gchar	*dir_ld_so_preload;

	gchar	*path_recover_info;
};

/*
#define GRAC_DAEMON_UDS_PATH		"/var/gooroom/grac-daemon"			// 권한 관리 시스템 모듈간 통신에 사용
#define GRAC_CFGFILE      "/etc/gooroom/grac.conf"					// 권한 관리 시스템 설정 파
*/

static struct _GracConfig GracConfig = {
	.max_path = PATH_MAX,
	.max_username = NAME_MAX,

	.dir_grac_data = "/etc/gooroom/grac.d",

	.dir_grac_rule = "/etc/gooroom/grac.d",

	.path_user_grac_rule    = "/etc/gooroom/grac.d/user.rules",
	.path_default_grac_rule = "/etc/gooroom/grac.d/default.rules",
	.path_grac_udev_map_org   = "/etc/gooroom/grac.d/grac-udev-rules.map",
	.path_grac_udev_map_local = "/etc/gooroom/grac.d/grac-udev-rules.map.local",

	.dir_udev_rules = "/etc/udev/rules.d",
	.path_udev_rules     = "/etc/udev/rules.d/grac-os.rules",
	.file_udev_rules     = "grac-os.rules",

	.path_hook_screenshooter_so   = "/usr/lib/x86_64-linux-gnu/libhook-screenshooter.so.0",
	.path_hook_screenshooter_conf = "/etc/gooroom/grac.d/hook-screenshooter.conf",

	.path_hook_clipboard_so   = "/usr/lib/x86_64-linux-gnu/libhook-clipboard.so.0",
	.path_hook_clipboard_conf = "/etc/gooroom/grac.d/hook-clipboard.conf",

	.file_ld_so_preload = "ld.so.preload",
	.path_ld_so_preload = "/etc/ld.so.preload",
	.dir_ld_so_preload = "/etc",

	.path_recover_info = "/etc/gooroom/grac.d/recover.udev"

};


// 존재하지 않는 경우에만 디렉토리 생성
static void _make_directory(char *dir)
{
	if (g_access(dir, F_OK) != 0) {
		if (mkdir(dir, 0755) < 0) {
			grm_log_error("mkdir : %s", c_strerror(-1));
		}
	}
}

// 권한 정보를 보관할 디렉토리 생성
static void _make_directory_tree(char *base_dir)
{
	char *make_dir = c_strdup(base_dir, GracConfig.max_path);
	char *ptr, *res;
	if (make_dir) {
		ptr = make_dir;
		if (*ptr == '/')
			ptr++;
		while (1) {
			if (*ptr == 0)
				break;
			res = c_strchr(ptr, '/', PATH_MAX);
			if (res == NULL) {
				_make_directory(make_dir);
				break;
			}
			else {
				*res = 0;
				_make_directory(make_dir);
				*res = '/';
				ptr = res + 1;
			}
		}
		free(make_dir);
	}
	else {
		_make_directory(base_dir);
	}
}


const gint	grac_config_max_length_path()
{
	return GracConfig.max_path;
}

const gint  grac_config_max_length_username()
{
	return GracConfig.max_username;
}

const gchar*	grac_config_dir_grac_data()
{
	if (g_access(GracConfig.dir_grac_data, F_OK) != 0)
		_make_directory_tree(GracConfig.dir_grac_data);

	return GracConfig.dir_grac_data;
}

const char*	grac_config_path_grac_rules(char* login_name)
{
	if (g_access(GracConfig.dir_grac_rule, F_OK) != 0)
		_make_directory_tree(GracConfig.dir_grac_rule);

	const char *path;
	int uid;

	if (c_strlen(login_name, 256) <= 0)
		uid = -1;
	else
		uid = sys_user_get_uid_from_name(login_name);

	if (uid < 0) {
		path = GracConfig.path_default_grac_rule;
	}
	else {
		path = GracConfig.path_user_grac_rule;
	}

	return path;
}

const char*	grac_config_path_user_grac_rules()
{
	if (g_access(GracConfig.dir_grac_rule, F_OK) != 0)
		_make_directory_tree(GracConfig.dir_grac_rule);

	return GracConfig.path_user_grac_rule;
}

const char*	grac_config_path_default_grac_rules()
{
	if (g_access(GracConfig.dir_grac_rule, F_OK) != 0)
		_make_directory_tree(GracConfig.dir_grac_rule);

	return GracConfig.path_default_grac_rule;
}

const char*	grac_config_path_udev_map_org()
{
	if (g_access(GracConfig.dir_grac_data, F_OK) != 0)
		_make_directory_tree(GracConfig.dir_grac_data);

	return GracConfig.path_grac_udev_map_org;
}

const char*	grac_config_path_udev_map_local()
{
	if (g_access(GracConfig.dir_grac_data, F_OK) != 0)
		_make_directory_tree(GracConfig.dir_grac_data);

	return GracConfig.path_grac_udev_map_local;
}

const char*	grac_config_path_udev_rules()
{
	if (g_access(GracConfig.dir_udev_rules, F_OK) != 0)
		_make_directory_tree(GracConfig.dir_udev_rules);

	return GracConfig.path_udev_rules;
}

const char*	grac_config_file_udev_rules()
{
	return GracConfig.file_udev_rules;
}

const gchar*	grac_config_path_hook_screenshooter_so()
{
	return GracConfig.path_hook_screenshooter_so;
}

const gchar*	grac_config_path_hook_screenshooter_conf()
{
	if (g_access(GracConfig.dir_grac_data, F_OK) != 0)
		_make_directory_tree(GracConfig.dir_grac_data);

	return GracConfig.path_hook_screenshooter_conf;
}

const gchar*	grac_config_path_hook_clipboard_so()
{
	return GracConfig.path_hook_clipboard_so;
}

const gchar*	grac_config_path_hook_clipboard_conf()
{
	if (g_access(GracConfig.dir_grac_data, F_OK) != 0)
		_make_directory_tree(GracConfig.dir_grac_data);

	return GracConfig.path_hook_clipboard_conf;
}

const gchar*	grac_config_path_ld_so_preload()
{
	return GracConfig.path_ld_so_preload;

}

const gchar*	grac_config_dir_ld_so_preload()
{
	return GracConfig.dir_ld_so_preload;
}

const gchar*	grac_config_file_ld_so_preload()
{
	return GracConfig.file_ld_so_preload;
}

int	grac_config_network_printer_port()
{
	return 515;
}

const gchar*	grac_config_path_recover_info()
{
	return GracConfig.path_recover_info;
}
