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
#include "grac_access.h"
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

#define BUF_SIZE 2048

static GMainLoop *loop;

// daemon 제어용 변수
struct _DaemonCtrl {

	GracAccess	*grac_access_data;

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

	DaemonCtrl.grac_access_data = grac_access_alloc();
	if (DaemonCtrl.grac_access_data == NULL) {
		grm_log_error("init_porc(): can't alloc grac_access");
		ret = FALSE;
	}

	return ret;
}

//  daemon 종료를 위한 마무리 작업
//   - 버퍼 해제
static void	term_proc()
{
	grac_access_free(&DaemonCtrl.grac_access_data);
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

static gboolean load_data_and_apply()
{
	gboolean done = FALSE;
	char 	username[1024], *puser;

	G_LOCK (load_apply_lock);

grm_log_debug("load_data_and_apply() : 11111");

	gboolean res, admin = FALSE;
	res = sys_user_get_login_name_by_api(username, sizeof(username));
	if (res == FALSE) {
		grm_log_error("load_data_and_apply() : can't get login name : assume guest");
		puser = NULL;
	}
	else {
		if (sys_user_is_in_group(username, "adm"))
			admin = TRUE;
		else if (sys_user_is_in_group(username, "sudo"))
			admin = TRUE;
		puser = username;
	}
grm_log_debug("load_data_and_apply() : 2222");


	grac_access_clear(DaemonCtrl.grac_access_data);

	char *path = (char*)grac_config_path_access();
	if (access(path, F_OK) == 0) {
		done = grac_access_load(DaemonCtrl.grac_access_data, path);
		if (done == FALSE) {
			grm_log_error("load_data_and_apply()  : load error");
		}
	}
	else {
		if (admin)
			grac_access_set_default_of_admin(DaemonCtrl.grac_access_data);
		else
			grac_access_set_default_of_guest(DaemonCtrl.grac_access_data);
		done = TRUE;
	}
grm_log_debug("load_data_and_apply() : 33333");

	if (done) {
		res = grac_access_apply(DaemonCtrl.grac_access_data);
		if (res == FALSE)
			grm_log_error("load_data_and_apply()  : apply error");
		done &= res;
grm_log_debug("load_data_and_apply()  : 33333 - 2");

//		res = grac_access_apply_by_user(DaemonCtrl.grac_access_data, puser);
//		if (res == FALSE)
//			grm_log_error("load_data_and_apply()  : apply by user error");
//		done &= res;
	}

grm_log_debug("load_data_and_apply() : 4444");
	G_UNLOCK (load_apply_lock);

grm_log_debug("load_data_and_apply() : 5555");
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

	//  load and apply
	load_data_and_apply();

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

	return G_SOURCE_REMOVE;
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


