/*
 ============================================================================
 Name        : grac_device_daemon.c
 Author      : 
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */

/**
 @mainpage grac-device-daemon
 @section intro 소개
    서비스 모드로 실행되며 주요 기능은 다음과 같다  \n
   - 권한에 따른 리소스 접근 여부를 시스템에 반영
   - 프로그램 실행을 수행하는 grac-agent 제어
  @section Program 프로그램명
   - grac-devd
 @section CREATEINFO 작성정보
      - 작성일 : 2017.04.05
 **/

/**
  @file  grac_device_daemon.c
  @brief main flow of grac-daemon
  @warning
  				원격지로부터 단말의 grac-daemon에 접속하는 기능은 지원하지 않는다.
  				 미비사항 \n
  				 권한 서버로부터 권한 정보를 가져와서 적용하는 기능 \n
  				 단말에 저장된 권한 정보 변경 여부 감지 기능
 */

#include "grac_device_daemon.h"
#include "grac_rule.h"
#include "grac_config.h"
#include "grm_log.h"
#include "cutility.h"

#include "sys_user.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <glib.h>
#include <glib-object.h>
#include <glib-unix.h>
#include <gio/gio.h>

#include <grmpycaller.h>

static GMainLoop *loop;

// daemon 제어용 변수
struct _DaemonCtrl {

	GracRule	*grac_rule;

} DaemonCtrl
= {
		NULL
};

static GMainLoop *loop;

/*
static int _gettid()
{
	return syscall(__NR_gettid);
}
*/

// daemon 수행에 필요한 준비 작업
//  버퍼 확보, 권한데이터 로드
static gboolean init_proc()
{
	gboolean ret = TRUE;
	memset(&DaemonCtrl, 0, sizeof(DaemonCtrl));

	DaemonCtrl.grac_rule = grac_rule_alloc();
	if (DaemonCtrl.grac_rule == NULL) {
		grm_log_error("init_porc(): can't alloc grac_access");
		ret = FALSE;
	}

	if (ret) {
		const char *path;

//	path = grac_config_path_user_grac_rules();
//	if (path)
//		unlink(path);

		path = grac_config_path_udev_rules();
		if (path)
			unlink(path);

		path = grac_config_path_ld_so_preload();
		if (path)
			unlink(path);
	}


	return ret;
}

//  daemon 종료를 위한 마무리 작업
//   - 버퍼 해제
static void	term_proc()
{
	grac_rule_free(&DaemonCtrl.grac_rule);
}

/**
@brief daemon 전체를 종료 시킨다.
@details
      daemon의 통신 thread를  종료 시키고 .
      daemon 자체를 종료 시킨다.
 */
void stop_daemon()
{
	g_main_loop_quit (loop);
}

G_LOCK_DEFINE_STATIC (load_apply_lock);


// 정책파일 무시, 모든 권한 부여
/*
static gboolean apply_admin_rule()
{
	gboolean done;

	done = grac_rule_apply_allow_all(DaemonCtrl.grac_rule);

	return done;
}
*/

// 특별히 추가적으로 할 일 없음
static gboolean apply_guest_rule()
{
	gboolean done;

	grm_log_debug("apply_guest_rule()");
	done = grac_rule_apply(DaemonCtrl.grac_rule);
	if (done == FALSE)
		grm_log_error("apply_guest_rule() : apply error");

	return done;
}

// 정책파일 사용, 권한 부여
static gboolean apply_grac_rule()
{
	gboolean done;

	grm_log_debug("apply_grac_rule()");
	done = grac_rule_apply(DaemonCtrl.grac_rule);
	if (done == FALSE)
		grm_log_error("apply_grac_rule() : apply error");

	return done;
}

static gboolean verify_grac_rule_file(char* path)
{
	gboolean done = FALSE;
	char	task[2048];
	char	*task_format;
	char	*result;

	task_format = "{\"task_name\":\"verify_signature\", \"file_name\":\"%s\"}";
	snprintf(task, sizeof(task), task_format, path);

	result = do_task(task);
	if (result) {
		// check result
		if (c_strstr(result, "\"SUCCESS\"", 1024)) {
			done = TRUE;
		}
		else {
			char *ptr, *msg = NULL;
			ptr = c_strstr(result, "\"FAIL\"", 1024);
			if (ptr) {
				ptr += c_strlen("\"FAIL\"", 256);
				ptr = c_strstr(ptr, "\"message\"", 1024);
				if (ptr)
					ptr += c_strlen("\"message\"", 256);
				if (ptr) {
					char *p;
					p = c_strchr(ptr+1, '\"', 1024);
					if (p)
						msg = c_strdup(p+1, 1024);
					if (msg) {
						p = c_strrchr(msg, '\"', 1024);
						if (p)
							*p = 0;
					}
				}
			}
			if (msg)
				grm_log_error("%s() : fail from do_task() : %s", __FUNCTION__, msg);
			else
				grm_log_error("%s() : fail from do_task()", __FUNCTION__);
		}
		free(result);
	}
	else {
		grm_log_error("%s() : no reply from do_task()", __FUNCTION__);
	}

	return done;
}

static const char *get_rule_path(int *user_rule)
{
	// user name check
/*
	char 	username[1024];
	int		uid;
	gboolean res, admin = FALSE;
	gboolean default_rule = FALSE;

	// 로그인 사용자 확인
	res = sys_user_get_login_name(username, sizeof(username));
	if (res == FALSE) {
		grm_log_error("load_data_and_apply() : can't get login name : assume guest");
		admin = FALSE;
		uid = -1;
		default_rule = TRUE;
	}
	else {
		if (sys_user_is_in_group(username, "adm"))
			admin = TRUE;
		else if (sys_user_is_in_group(username, "sudo"))
			admin = TRUE;
		else
			admin = FALSE;
		uid = sys_user_get_uid_from_name(username);
		if (uid < 0)
			default_rule = TRUE;
		else
			default_rule = FALSE;
	}
*/

	const char *path = NULL;
	*user_rule = -1;

	path = grac_config_path_user_grac_rules();
	if (path) {
		if (access(path, F_OK) != 0) {
			grm_log_warning("%s() : not found the user rule", __FUNCTION__);
			path = NULL;
		}
		else {
			*user_rule = 1;
		}
	}

	if (path == NULL) {
		path = grac_config_path_default_grac_rules();
		if (path) {
			if (access(path, F_OK) != 0) {
				grm_log_error("%s() : not found the default rule", __FUNCTION__);
				path = NULL;
			}
			else {
				*user_rule = 0;
			}
		}
	}

	return path;
}


static gboolean load_data_and_apply()
{
	gboolean done = FALSE;
	const char	*rule_path;
	int		user_rule;

	G_LOCK (load_apply_lock);

grm_log_debug("load_data_and_apply() : start");

	rule_path = get_rule_path(&user_rule);

	if (rule_path && user_rule == 1) {
		gboolean res = verify_grac_rule_file((char*)rule_path);
		if (res == FALSE) {
			grm_log_error("load_data_and_apply() : not verified grac rule file [%s]", rule_path);
			rule_path = NULL;
		}
	}

	grac_rule_clear(DaemonCtrl.grac_rule);

	if (rule_path != NULL) {
		gboolean res;
		res = grac_rule_load(DaemonCtrl.grac_rule, (gchar*)rule_path);
		if (res == FALSE) {
			grm_log_error("%s() : load rule error : %s", __FUNCTION__, rule_path);
			grac_rule_set_default_of_guest(DaemonCtrl.grac_rule);
			done = apply_guest_rule();	// 정책파일 오류,  최소 권한 부여
		}
		else {
			done = apply_grac_rule();
		}
	}
	else { // 정책파일 없음
//	if (admin == TRUE) {
//			grac_rule_set_default_of_admin(DaemonCtrl.grac_rule);
//			done = apply_admin_rule();	// 모든 권한 부여
//		}
//	else {
			grac_rule_set_default_of_guest(DaemonCtrl.grac_rule);
			done = apply_guest_rule();	// 최소 권한 부여
//		}
	}

	G_UNLOCK (load_apply_lock);

	grm_log_debug("load_data_and_apply() : end");
	return done;
}


static void
on_bus_acquired (GDBusConnection  *connection,
                 const gchar      *name,
                 gpointer          user_data)
{
	int		res = EXIT_SUCCESS;
	char	*msg;

	grm_log_debug("on_bus_acquired() : %s", name);

	if (init_proc() == FALSE) {
		msg = "Can't start daemon : ini_proc()";
		printf("====================%s\n", msg);
		grm_log_error(msg);
		res = EXIT_FAILURE;
		goto STOP;
	}

	// 외부에서 요청시에만 접근정책 적용
	// 이시점에서는 아무것도 수행하지 않는다
	// load and apply
	// load_data_and_apply();

STOP:
	if (res == EXIT_FAILURE)
		g_main_loop_quit (loop);
}

static void on_name_acquired (GDBusConnection  *connection,
              const gchar      *name,
              gpointer          user_data)
{
	grm_log_debug("on_name_acquired() : %s", name);
}


static void
on_name_lost (GDBusConnection  *connection,
              const gchar      *name,
              gpointer          user_data)
{
	grm_log_debug("on_name_lost()");

	g_main_loop_quit (loop);
}

static gboolean
on_signal_quit (gpointer data)
{
	grm_log_debug("on_signal_quit()");

	g_main_loop_quit (data);

	return FALSE;
}

static gboolean on_signal_reload (gpointer data)
{
	grm_log_debug("on_signal_reload()");

	load_data_and_apply();

	return G_SOURCE_CONTINUE;
}

/**
@brief main function
@details
     서비스 모드 (daemon mode) 로 전환시키고
     실질적인 main loop인  g_main_loop_run()가 종료 될 때까지 기다린다.
@return integer 0:성공  -1: 실패
 */

int main(void)
{
	guint owner_id;

#if !GLIB_CHECK_VERSION (2, 35, 3)
	g_type_init ();
#endif

	owner_id = g_bus_own_name (G_BUS_TYPE_SYSTEM,
			"kr.gooroom.GRACDEVD",
			G_BUS_NAME_OWNER_FLAGS_NONE,
			on_bus_acquired,
			on_name_acquired,
			on_name_lost,
			NULL,
			NULL);

	loop = g_main_loop_new (NULL, FALSE);

	g_unix_signal_add (SIGINT, on_signal_quit, loop);
	g_unix_signal_add (SIGTERM, on_signal_quit, loop);

	// 2017.07.03 service reload signal
	g_unix_signal_add (SIGHUP, on_signal_reload, loop);

	g_main_loop_run (loop);
	g_main_loop_unref (loop);

	g_bus_unown_name (owner_id);

	term_proc();

	grm_log_debug("end of main");

	return 0;
}


