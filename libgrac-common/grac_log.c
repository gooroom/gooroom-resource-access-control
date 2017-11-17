/*
 * Copyright (c) 2015 - 2017 gooroom <gooroom@gooroom.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * grac_log.c
 *
 *  Created on: 2017. 5. 17.
 *      Author: gooroom@gooroom.kr
 */


/**
 @mainpage
 @section intro 소개
   - 구름 권한 시스템용 라이브러리  \n
   - 구성  \n
   		1) 어플리케이션용  - 구름 권한 시스템을 이용하고자하는 어플리케이션이 사용 (예)브라우저 \n
		    grac_control\n
		2) 구름 플랫폼용 - 구름OS 상의 어플리케이션이 사용하는 라이브러리, 특정 권한 모델과는 독립적이다.\n
		    grac_log \n
		3) 구름 권한 시스템 -  구름권한제어 시스템이 사용하는 라이브러리 (예) grac-daemon\n
		     grac-*** (grac-control을 제외한 모든 것) \n
		4) 시스템 의존성 부분 - linux system 의존성 라이브러리,   C 언어용 라이브러리\n
			sys_***,  c_***
 @section Program 프로그램명
   - libgrac
 @section CREATEINFO 작성정보
      - 작성일 : 2017.04.05
 **/

/**
  @file 	 	grac_log.c
  @brief		구름 플랫폼에서 개발되는 어플리케이션을 위한 로그 처리
  @details	로그의 위치는 /var/log/gooroom/grac.log에 생성된다.	\n
  				로그의 레벨은  DEBUG,  INFO,  NOTICE,  WARNING,  ERROR 로 분리된다 \n
   				헤더파일 :  	grac_log.h	\n
  				라이브러리:	libgrac.so
 */

// 주의 : grac_log의 중첩 호출을 방지하기 위하여 이곳에서는 시스템에서 제공하는 함수만 사용한다.

#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "grac_log.h"
#include "sys_etc.h"
#include "cutility.h"

typedef enum
{
  GRM_LOG_LEVEL_DEBUG,
  GRM_LOG_LEVEL_INFO,
  GRM_LOG_LEVEL_NOTICE,
  GRM_LOG_LEVEL_WARNING,
  GRM_LOG_LEVEL_ERROR
} GrmLogLevel;


G_LOCK_DEFINE_STATIC(grac_log_lock);

static gchar G_AppName[128] = {0};
static int   G_AppNameSize = sizeof(G_AppName);

void grac_log_set_name(gchar *appname)
{
	c_strcpy(G_AppName, appname, G_AppNameSize);
}


#define PATH_LOG_BASE1 "/var/log"
#define PATH_LOG_BASE2 "/var/log/gooroom"
#define PATH_LOG_FILE  "/var/log/gooroom/grac.log"
#define PATH_LOG_BACK  "/var/log/gooroom/grac.log.bak"

#define LOG_FILE_MAX_SIZE (100*1024*1024)		// 100 M

static gboolean _limit_log_file_size()
{
		if (getuid() != 0)
			return FALSE;

		int res, size;
		struct stat finfo;
		res = stat(PATH_LOG_FILE, &finfo);
		if (res != 0)
			return FALSE;

		size = finfo.st_size;
		if (size > LOG_FILE_MAX_SIZE) {
				unlink(PATH_LOG_BACK);
				rename(PATH_LOG_FILE, PATH_LOG_BACK);
				FILE *fp = g_fopen(PATH_LOG_FILE, "w");
				if (fp)
					fclose(fp);
				if (g_access(PATH_LOG_FILE, F_OK) == 0)
					g_chmod(PATH_LOG_FILE, 0766);
		}

		res = stat(PATH_LOG_FILE, &finfo);
		if (res != 0)
			return FALSE;

		if ((finfo.st_mode & S_IWGRP) == 0)
			return FALSE;
		if ((finfo.st_mode & S_IWOTH) == 0)
			return FALSE;


	return TRUE;
}

// if syslog acts correctly, ignore this funtion.
static void _do_file_log(char *msg)
{
	GTimeVal current;
	struct tm *now_tm;
	gchar time_str[128];
	g_get_current_time(&current);
	now_tm = localtime(&current.tv_sec);
	strftime(time_str, sizeof(time_str), "%m.%d %H:%M:%S", now_tm);

	char	module[1024];
	if (sys_get_current_process_name(module, sizeof(module)) == FALSE)
		c_strcpy(module, "unknown", sizeof(module));

	static gboolean check_dir = FALSE;
	if (check_dir == FALSE) {
		if (g_access(PATH_LOG_BASE1, F_OK) != 0)
			mkdir(PATH_LOG_BASE1, 0777);
		if (g_access(PATH_LOG_BASE2, F_OK) != 0)
			mkdir(PATH_LOG_BASE2, 0777);
		if (g_access(PATH_LOG_FILE, F_OK) != 0) {
			FILE *fp = g_fopen(PATH_LOG_FILE, "w");
			if (fp)
				fclose(fp);
		}
		if (g_access(PATH_LOG_FILE, F_OK) == 0)
			g_chmod(PATH_LOG_FILE, 0766);

		check_dir = _limit_log_file_size();
	}


	FILE *fp = g_fopen(PATH_LOG_FILE, "a");
	if (fp) {
		int n = c_strlen(msg, 2048);
		int m_sec = (int)(current.tv_usec/100);  // 10**-4 second
		if (n > 0 && msg[n-1] == '\n')
			g_fprintf(fp, "%s.%04d [%3s %d] %s", time_str, m_sec, module, getpid(), msg);
		else
			g_fprintf(fp, "%s.%04d [%3s %d] %s\n", time_str, m_sec, module, getpid(), msg);
		fclose(fp);
	}
}

static void _do_sys_log(GrmLogLevel level, char *message)
{
	static gboolean log_opened = FALSE;
	gint log_priority;

	if (log_opened == FALSE) {
	  openlog(NULL, LOG_CONS | LOG_NDELAY | LOG_PID, LOG_USER);
	  log_opened = TRUE;
	}

	switch (level) {
	case GRM_LOG_LEVEL_DEBUG:
		log_priority = LOG_DEBUG;
		break;

	case GRM_LOG_LEVEL_INFO:
    log_priority = LOG_INFO;
    break;

	case GRM_LOG_LEVEL_NOTICE:
		log_priority = LOG_NOTICE;
		break;

	case GRM_LOG_LEVEL_WARNING:
		log_priority = LOG_WARNING;
		break;

	case GRM_LOG_LEVEL_ERROR:
		log_priority = LOG_ERR;
		break;

	default:
		g_assert_not_reached();
		break;
	}

	if (message != NULL) {
		syslog(log_priority, "%s", message);
	}

	// closelog();  // calling is optional
}

static void _grm_do_log(GrmLogLevel level, gchar* format, va_list var_args)
{
	G_LOCK(grac_log_lock);

	gchar *kind_str;
	gchar *message;

	switch (level) {
	case GRM_LOG_LEVEL_DEBUG:
		kind_str = "Debug";
		break;

	case GRM_LOG_LEVEL_INFO:
    kind_str = "Info";
    break;

	case GRM_LOG_LEVEL_NOTICE:
		kind_str = "Notice";
		break;

	case GRM_LOG_LEVEL_WARNING:
		kind_str = "Warning";
		break;

	case GRM_LOG_LEVEL_ERROR:
		kind_str = "Error";
		break;

	default:
		g_assert_not_reached();
		break;
	}

	char	new_format[1024];
	char	sys_message[1024];
	int		new_size;

	new_size = c_strlen(G_AppName, G_AppNameSize)+2
						+ c_strlen(kind_str, G_AppNameSize)+2
						+ c_strlen(format, G_AppNameSize)
						+ 1;
	if (new_size > sizeof(new_format)) {
		message = g_strdup_vprintf(format, var_args);
	} else {
		if (G_AppName[0] == 0)
			g_snprintf(new_format, sizeof(new_format), "%s: %s", kind_str, format);
		else
			g_snprintf(new_format, sizeof(new_format), "%s: %s: %s", G_AppName, kind_str, format);
		message = g_strdup_vprintf(new_format, var_args);
		if (message != NULL) {
			if (G_AppName[0] == 0)
				g_snprintf(sys_message, sizeof(sys_message), "app=\"GRAC\" msg=\"%s\"", message);
			else
				g_snprintf(sys_message, sizeof(sys_message), "app=\"%s\" msg=\"%s\"", G_AppName, message);
		}
	}

	if (message != NULL) {
		_do_sys_log(level, sys_message);
		_do_file_log(message);

		g_free(message);
	}

	G_UNLOCK(grac_log_lock);
}

void grac_log(GrmLogLevel level, gchar *format, ...)
{
	va_list var_args;
	va_start(var_args, format);
	_grm_do_log(level, format, var_args);
	va_end(var_args);
}

/**
 @brief   DEBUG 레벨 로그 저장
 @param	format 	출력 포맷
 */
void grac_log_debug(gchar *format, ...)
{
	va_list var_args;
	va_start(var_args, format);
	_grm_do_log(GRM_LOG_LEVEL_DEBUG, format, var_args);
	va_end(var_args);
}

/**
 @brief   INFO 레벨 로그 저장
 @param	format 	출력 포맷
 */
void grac_log_info(gchar *format, ...)
{
	va_list var_args;
	va_start(var_args, format);
	_grm_do_log(GRM_LOG_LEVEL_INFO, format, var_args);
	va_end(var_args);
}

/**
 @brief  NOTICE 레벨 로그 저장
 @param	format 	출력 포맷
 */
void grac_log_notice(gchar *format, ...)
{
	va_list var_args;
	va_start(var_args, format);
	_grm_do_log(GRM_LOG_LEVEL_NOTICE, format, var_args);
	va_end(var_args);
}

/**
 @brief   WARNING 레벨 로그 저장
 @param	format 	출력 포맷
 */
void grac_log_warning(gchar *format, ...)
{
	va_list var_args;
	va_start(var_args, format);
	_grm_do_log(GRM_LOG_LEVEL_WARNING, format, var_args);
	va_end(var_args);
}

/**
 @brief   ERROR 레벨 로그 저장
 @param	format 	출력 포맷
 */
void grac_log_error(gchar *format, ...)
{
	va_list var_args;
	va_start(var_args, format);
	_grm_do_log(GRM_LOG_LEVEL_ERROR, format, var_args);
	va_end(var_args);
}
