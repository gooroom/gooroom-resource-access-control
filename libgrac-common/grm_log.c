/*
 * grm_log.c
 *
 *  Created on: 2016. 5. 17.
 *      Author: user
 */

/**
 @mainpage
 @section intro 소개
   - 구름 권한 시스템용 라이브러리  \n
   - 구성  \n
   		1) 어플리케이션용  - 구름 권한 시스템을 이용하고자하는 어플리케이션이 사용 (예)브라우저 \n
		    grac_control\n
		2) 구름 플랫폼용 - 구름OS 상의 어플리케이션이 사용하는 라이브러리, 특정 권한 모델과는 독립적이다.\n
		    grm_log \n
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
  @file 	 	grm_log.c
  @brief		구름 플랫폼에서 개발되는 어플리케이션을 위한 로그 처리
  @details	로그의 위치는 /var/log/gooroom/grac.log에 생성된다.	\n
  				로그의 레벨은  DEBUG,  INFO,  NOTICE,  WARNING,  ERROR 로 분리된다 \n
   				헤더파일 :  	grm_log.h	\n
  				라이브러리:	libgrac.so
 */

// 주의 : grm_log의 중첩 호출을 방지하기 위하여 이곳에서는 시스템에서 제공하는 함수만 사용한다.

#include "grm_log.h"
#include "sys_etc.h"

#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef enum
{
  GRM_LOG_LEVEL_DEBUG,
  GRM_LOG_LEVEL_INFO,
  GRM_LOG_LEVEL_NOTICE,
  GRM_LOG_LEVEL_WARNING,
  GRM_LOG_LEVEL_ERROR
} GrmLogLevel;

///-------------
// 모두 지원 안 함
//#define GRM_LOG_DEBUG  (args...)  grm_log(GRM_LOG_LEVEL_DEBUG, args)
//#define GRM_LOG_INFO   (args...)  grm_log(GRM_LOG_LEVEL_INFO, ##args)
//#define GRM_LOG_NOTICE (args...)  grm_log(GRM_LOG_LEVEL_NOTICE, __VA_ARGS__)
///-------------

G_LOCK_DEFINE_STATIC (grm_log_lock);

static gchar G_AppName[128] = {0};

void grm_log_set_name (gchar *appname)
{
	strncpy(G_AppName, appname, sizeof(G_AppName));
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
				res = unlink(PATH_LOG_BACK);
				res = rename(PATH_LOG_FILE, PATH_LOG_BACK);
				FILE *fp = fopen(PATH_LOG_FILE, "w");
				if (fp)
					fclose(fp);
				if (access(PATH_LOG_FILE, F_OK) == 0)
					chmod(PATH_LOG_FILE, 0766);
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
	g_get_current_time (&current);
	now_tm = localtime (&current.tv_sec);
	strftime (time_str, sizeof(time_str), "%m.%d %H:%M:%S", now_tm);

	char	module[1024];
	if (sys_get_current_process_name(module, sizeof(module)) == FALSE)
		strcpy(module, "unknown");
/*
	int		res;
	memset(module, 0, sizeof(module));
	res = readlink("/proc/self/exe", module, sizeof(module)-1);
	if (res < 0) {
		module[0] = 0;
	}
	else {
		char *ptr;
		ptr = strrchr(module, '/');
		if (ptr != NULL)
			strcpy(module, ptr+1);
	}
*/


	static gboolean check_dir = FALSE;
	if (check_dir == FALSE) {
		if (access(PATH_LOG_BASE1, F_OK) != 0)
			mkdir(PATH_LOG_BASE1, 0777);
		if (access(PATH_LOG_BASE2, F_OK) != 0)
			mkdir(PATH_LOG_BASE2, 0777);
		if (access(PATH_LOG_FILE, F_OK) != 0) {
			FILE *fp = fopen(PATH_LOG_FILE, "w");
			if (fp)
				fclose(fp);
		}
		if (access(PATH_LOG_FILE, F_OK) == 0)
			chmod(PATH_LOG_FILE, 0766);

		check_dir = _limit_log_file_size();
	}


	FILE *fp = fopen(PATH_LOG_FILE, "a");
	if (fp) {
		int n = strlen(msg);
		int m_sec = (int)(current.tv_usec/100);  // 10**-4 second
		if (n > 0 && msg[n-1] == '\n')
			fprintf(fp, "%s.%04d [%3s %d] %s", time_str, m_sec, module, getpid(), msg);
		else
			fprintf(fp, "%s.%04d [%3s %d] %s\n", time_str, m_sec, module, getpid(), msg);
		fclose(fp);
	}
}

static void _do_sys_log(GrmLogLevel level, char *message)
{
	static gboolean log_opened = FALSE;
	gint log_priority;

	if (log_opened == FALSE) {
	  openlog (NULL, LOG_CONS | LOG_NDELAY | LOG_PID, LOG_USER);
	  log_opened = TRUE;
	}

	switch (level)
	{
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
		g_assert_not_reached ();
		break;
	}

	if (message != NULL) {
		syslog (log_priority, "%s", message);
	}

	// closelog();  // calling is optional
}

static void _grm_do_log(GrmLogLevel level, gchar* format, va_list var_args)
{
	G_LOCK (grm_log_lock);

//	static gboolean version_out = FALSE;
//	static char*    log_version = "****** libgrac version 2016.12.01 (001) *****";
	gchar *kind_str;
	gchar *message;

	switch (level)
	{
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
		g_assert_not_reached ();
		break;
	}

	char	new_format[1024];
	int		new_size;

	new_size = strlen(G_AppName)+2 + strlen(kind_str)+2 + strlen(format) + 1;
	if (new_size > sizeof(new_format)) {
		message = g_strdup_vprintf (format, var_args);
	}
	else {
		if (G_AppName[0] == 0)
			sprintf(new_format, "%s: %s", kind_str, format);
		else
			sprintf(new_format, "%s: %s: %s", G_AppName, kind_str, format);
		message = g_strdup_vprintf (new_format, var_args);
	}

//	if (version_out == FALSE) {
//		_do_sys_log(level, log_version);
//		_do_file_log(log_version);
//		version_out = TRUE;
//		}

	if (message != NULL) {
		_do_sys_log (level, message);
		_do_file_log(message);

		g_free (message);
	}

	G_UNLOCK (grm_log_lock);

}

void grm_log(GrmLogLevel level, gchar *format, ...)
{
	va_list var_args;
	va_start (var_args, format);
	_grm_do_log(level, format, var_args);
	va_end (var_args);
}

/**
 @brief   DEBUG 레벨 로그 저장
 @param	format 	출력 포맷
 */
void grm_log_debug  (gchar *format, ...)
{
	va_list var_args;
	va_start (var_args, format);
	_grm_do_log(GRM_LOG_LEVEL_DEBUG, format, var_args);
	va_end (var_args);
}

/**
 @brief   INFO 레벨 로그 저장
 @param	format 	출력 포맷
 */
void grm_log_info   (gchar *format, ...)
{
	va_list var_args;
	va_start (var_args, format);
	_grm_do_log(GRM_LOG_LEVEL_INFO, format, var_args);
	va_end (var_args);
}

/**
 @brief  NOTICE 레벨 로그 저장
 @param	format 	출력 포맷
 */
void grm_log_notice (gchar *format, ...)
{
	va_list var_args;
	va_start (var_args, format);
	_grm_do_log(GRM_LOG_LEVEL_NOTICE, format, var_args);
	va_end (var_args);
}

/**
 @brief   WARNING 레벨 로그 저장
 @param	format 	출력 포맷
 */
void grm_log_warning(gchar *format, ...)
{
	va_list var_args;
	va_start (var_args, format);
	_grm_do_log(GRM_LOG_LEVEL_WARNING, format, var_args);
	va_end (var_args);
}

/**
 @brief   ERROR 레벨 로그 저장
 @param	format 	출력 포맷
 */
void grm_log_error  (gchar *format, ...)
{
	va_list var_args;
	va_start (var_args, format);
	_grm_do_log(GRM_LOG_LEVEL_ERROR, format, var_args);
	va_end (var_args);
}
