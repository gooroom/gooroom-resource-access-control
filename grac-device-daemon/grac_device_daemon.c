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
#include "grac_adjust_udev_rule.h"
#include "grm_log.h"
#include "cutility.h"

#include "sys_file.h"
#include "sys_etc.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib-object.h>
#include <glib-unix.h>
#include <glib/gstdio.h>
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

		grac_rule_pre_process();
	}


	return ret;
}

//  daemon 종료를 위한 마무리 작업
//   - 버퍼 해제
static void	term_proc()
{
	grac_rule_free(&DaemonCtrl.grac_rule);

	grac_rule_post_process();
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


static gboolean verify_grac_rule_file(char* path)
{
	gboolean done = FALSE;
	char	task[2048];
	char	*task_format;
	char	*result;

	task_format = "{\"task_name\":\"verify_signature\", \"file_name\":\"%s\"}";
	g_snprintf(task, sizeof(task), task_format, path);

	result = do_task(task);
	if (result) {
		// check result
		if (c_strstr(result, "\"SUCCESS\"", 1024)) {
			done = TRUE;
		}
		else {
			char 	*msg = NULL;
/*
			char	*ptr;
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
*/
			msg = result;
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

static const char *check_rule_path(gboolean user)
{
	char *path = NULL;

	if (user)
		path = (char*)grac_config_path_user_grac_rules();
	else
		path = (char*)grac_config_path_default_grac_rules();

	if (path) {
		if (sys_file_is_existing(path) == FALSE) {
			grm_log_warning("%s() : not found rule [%s]", __FUNCTION__, path);
			path = NULL;
		}
		else {
			if (sys_file_get_length(path) <= 0) {
				grm_log_warning("%s() : empty rule file", __FUNCTION__, path);
				path = NULL;
			}
		}
	}

	if (path && user) {
		gboolean res = verify_grac_rule_file((char*)path);
		if (res == FALSE) {
			grm_log_error("%s() : not verified rule file [%s]", __FUNCTION__, path);
			path = NULL;
		}
	}

	return path;
}


static gboolean adjust_udev_rule_map_file()
{
	gboolean done;
	char*	map_org;
	char*	map_local;
	struct stat st_org;
	struct stat st_local;
	int		res;


	map_org = (char*)grac_config_path_udev_map_org();
	map_local = (char*)grac_config_path_udev_map_local();

	res = stat(map_org, &st_org);
	if (res != 0) {
		grm_log_error("%s() : %s : %s", __FUNCTION__, map_org, strerror(errno));
		return FALSE;
	}

	res = stat(map_local, &st_local);
	if (res == 0) {
		if (st_local.st_mtim.tv_sec > st_org.st_mtim.tv_sec)
			return TRUE;
		if (st_local.st_mtim.tv_sec == st_org.st_mtim.tv_sec &&
				st_local.st_mtim.tv_nsec > st_org.st_mtim.tv_nsec)
			return TRUE;
	}
	else {
		if (errno != ENOENT) {
			grm_log_error("%s() : %s : %s", __FUNCTION__, map_org, strerror(errno));
			return FALSE;
		}
	}

	done = grac_adjust_udev_rule_file(map_org, map_local);
	if (done == FALSE)
		grm_log_error("%s() : adjust map error", __FUNCTION__);

	return done;
}

static void _recover_configurationValue(char *buf, int buf_size)
{
	char	*ptr;
	int		i, n, ch, cnt;
	char	*attr = "bConfigurationValue";

	ptr = c_stristr(buf, attr, buf_size);
	if (ptr) {
		ptr += c_strlen(attr, 256);
		ptr++;
		if (*ptr >= '0' && *ptr <= '9') {
			cnt = *ptr - '0';
			ptr++;
		}
		else {
			cnt = 0;
		}

		ptr = c_stristr(buf, "D=", buf_size);
		if (ptr) {
			ptr += 2;
			char	dev_path[512];
			for (i=0; i<sizeof(dev_path)-1; i++) {
				ch = ptr[i] & 0x0ff;
				if (ch <= 0x020)
					break;
				dev_path[i] = ch;
			}
			if (i>0 && dev_path[i-1] == '/')
				i--;
			dev_path[i] = 0;

			if (cnt > 0) {
				n = i;
				for (i=n-1; i>=0; i--) {
					if (dev_path[i] == '/') {
						dev_path[i] = 0;
						cnt--;
						if (cnt == 0)
							break;
					}
				}
			}

			n = c_strlen(dev_path, sizeof(dev_path));
			if (n > 0) {
				char	cmd[1024];
				g_snprintf(cmd, sizeof(cmd), "echo 1 > %s/%s", dev_path, attr);
				if (sys_run_cmd_no_output (cmd, "grac-recover") == FALSE)
					grm_log_error("command error  : %s", cmd);
				else
					grm_log_info("set %s for %s", attr, dev_path);
			}
			else {
				grm_log_error("invalid recover information : %s", buf);
			}
		}
	}
}


static gboolean recover_applied_device()
{
	gboolean done = TRUE;
	const char *path;

	grm_log_debug("%s() : start", __FUNCTION__);

	path = grac_config_path_recover_info();
	if (path == NULL) {
		grm_log_debug("%s() : end : no file name", __FUNCTION__);
		return TRUE;
	}

	FILE	*fp;
	char	buf[1024];
	gboolean rescan = FALSE;
	gboolean trigger = FALSE;
	char	*cmd;
	gboolean res;

	fp = g_fopen(path, "r");
	if (fp == NULL) {
		grm_log_debug("%s() : end : no file", __FUNCTION__);
		return TRUE;
	}

	// delete the created udev rule
	// 복구를 위한 udev rule 필요시 이 지점에서 생성
	path = grac_config_path_udev_rules();
	if (path != NULL)
		unlink(path);
	cmd = "udevadm control --reload";
	res = sys_run_cmd_no_output (cmd, "apply-rule");
	if (res == FALSE)
		grm_log_error("%s(): can't run %s", __FUNCTION__, cmd);


	// todo : 정밀 검토
	while (1) {
		if (fgets(buf, sizeof(buf), fp) == NULL)
			break;
		if (c_stristr(buf, "rescan", sizeof(buf)))
			rescan = TRUE;
		_recover_configurationValue(buf, sizeof(buf));
	}
	fclose(fp);

	path = grac_config_path_recover_info();
	unlink(path);

	// rescan
	if (rescan) {
		cmd = "echo 1 > /sys/bus/pci/rescan";
		res = sys_run_cmd_no_output (cmd, "apply-rule");
		if (res == FALSE)
			grm_log_error("%s(): can't run %s", __FUNCTION__, cmd);
		done &= res;
	}

	// trigger
	if (trigger) {
		cmd = "udevadm control --reload";
		res = sys_run_cmd_no_output (cmd, "apply-rule");
		if (res == FALSE)
			grm_log_error("%s(): can't run %s", __FUNCTION__, cmd);
		done &= res;

		cmd = "udevadm trigger -c add";
		res = sys_run_cmd_no_output (cmd, "apply-rule");
		if (res == FALSE)
			grm_log_error("%s(): can't run %s", __FUNCTION__, cmd);
		done &= res;
	}

	// 복구를 위한 udev rule 생성된 경우 이 지점에서 삭제

	grm_log_debug("%s() : end", __FUNCTION__);

	return done;
}


static gboolean load_data_and_apply()
{
	gboolean done = FALSE;
	gboolean load = FALSE;
	const char	*rule_path;

	G_LOCK (load_apply_lock);

	grm_log_info("Apply grac-rule");
	grm_log_debug("load_data_and_apply() : start");

	if (adjust_udev_rule_map_file() == FALSE)
		grm_log_error("%s() : can't adjust map file", __FUNCTION__);

	// 2017.10.12
	if (recover_applied_device() == FALSE)
		grm_log_error("%s() : can't recover devices", __FUNCTION__);

	// try to load user.rules
	grac_rule_clear(DaemonCtrl.grac_rule);
	rule_path = check_rule_path(TRUE);
	if (rule_path) {
		load = grac_rule_load(DaemonCtrl.grac_rule, (gchar*)rule_path);
		if (load == FALSE) {
			grm_log_error("%s() : load rule error : %s", __FUNCTION__, rule_path);
		}
	}

	// try to load default
	if (load == FALSE) {
		grac_rule_clear(DaemonCtrl.grac_rule);
		rule_path = check_rule_path(FALSE);
		if (rule_path) {
			load = grac_rule_load(DaemonCtrl.grac_rule, (gchar*)rule_path);
			if (load == FALSE) {
				grm_log_error("%s() : load rule error : %s", __FUNCTION__, rule_path);
			}
		}
	}

	// if load, error make default
	if (load == FALSE) {
		grac_rule_set_default_of_guest(DaemonCtrl.grac_rule);  // 정책파일 오류,  최소 권한 부여
	}

	//  정책적용
	done = grac_rule_apply(DaemonCtrl.grac_rule);
	if (done == FALSE)
		grm_log_error("%s() : apply error", __FUNCTION__);

	G_UNLOCK (load_apply_lock);

	grm_log_info("Apply grac-rule : result = %d", (int)done);
	grm_log_debug("%s() : result = %d", __FUNCTION__, (int)done);

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

	// 2017.10.20
	if (recover_applied_device() == FALSE)
		grm_log_error("%s() : can't recover devices", __FUNCTION__);

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


