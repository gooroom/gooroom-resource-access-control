
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <glib.h>

typedef enum
{
  GRM_LOG_LEVEL_DEBUG,
  GRM_LOG_LEVEL_INFO,
  GRM_LOG_LEVEL_NOTICE,
  GRM_LOG_LEVEL_WARNING,
  GRM_LOG_LEVEL_ERROR
} GrmLogLevel;


G_LOCK_DEFINE_STATIC (grm_log_lock);


static void _do_sys_log(GrmLogLevel level, char *message)
{
	static gboolean log_opened = FALSE;
	gint log_priority;

	if (log_opened == FALSE) {
	  openlog (NULL, LOG_CONS | LOG_NDELAY | LOG_PID, LOG_DAEMON);	// LOG_USER
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

char G_AppName[256] = { 0 };

static void _grm_do_log(GrmLogLevel level, gchar* format, va_list var_args)
{
	G_LOCK (grm_log_lock);

	static gboolean version_out = FALSE;
	static char*    log_version = "****** libgrac version 2016.12.01 (001) *****";
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

	if (message != NULL) {

		_do_sys_log (level, message);

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

int	test_sys_log()
{
	grm_log_debug  ("GRAC This is a debug log");
	grm_log_info   ("GRAC This is a info log");
	grm_log_notice ("GRAC This is a notice log");
	grm_log_warning("GRAC This is a warning log");
	grm_log_error  ("GRAC This is a error log");

	return 0;
}
