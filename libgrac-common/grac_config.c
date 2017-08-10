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
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

struct _GracConfig {
	gint max_path;
	gint	max_username;
	gint	max_url;
	gint	max_packet;

	gchar	*data_base_dir;
	gchar	*data_base_dir_mark;

	gchar	*path_access;
	gchar	*path_url_list;

	gchar *path_browser_app;
};

/*
#define GRAC_DAEMON_UDS_PATH		"/var/gooroom/grac-daemon"			// 권한 관리 시스템 모듈간 통신에 사용
#define GRAC_CFGFILE      "/etc/gooroom/grac.conf"					// 권한 관리 시스템 설정 파
*/

static struct _GracConfig GracConfig = {
	.max_path = PATH_MAX,
	.max_username = NAME_MAX,
	.max_url = 2048,
	.max_packet = 4096,

	.data_base_dir      = "/etc/gooroom",
	.data_base_dir_mark = "/etc/gooroom/",
	.path_access        = "/etc/gooroom/grac-access.json",
	.path_url_list      = "/etc/gooroom/grac-url-list.json",

	.path_browser_app = "/usr/bin/gooroom-browser"
};


// 존재하지 않는 경우에만 디렉토리 생성
static void _make_directory(char *dir)
{
	if (access(dir, F_OK) != 0) {
		if (mkdir(dir, 0755) < 0) {
			grm_log_error("mkdir : %s", strerror(errno));
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
			res = strchr(ptr, '/');
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

const gint  grac_config_max_length_packet()
{
	return GracConfig.max_packet;
}

const gint  grac_config_max_length_url()
{
	return GracConfig.max_url;
}

const gchar*	grac_config_data_base_directory(gboolean end_mark)
{
	if (access(GracConfig.data_base_dir, F_OK) != 0)
		_make_directory_tree(GracConfig.data_base_dir);

	if (end_mark)
		return GracConfig.data_base_dir_mark;
	else
		return GracConfig.data_base_dir;
}

const char*	grac_config_path_access()
{
	if (access(GracConfig.data_base_dir, F_OK) != 0)
		_make_directory_tree(GracConfig.data_base_dir);

	return GracConfig.path_access;
}

const char*	grac_config_path_url_list()
{
	if (access(GracConfig.data_base_dir, F_OK) != 0)
		_make_directory_tree(GracConfig.data_base_dir);

	return GracConfig.path_url_list;
}

const gchar*	grac_config_path_browser_app()
{
	return GracConfig.path_browser_app;
}
