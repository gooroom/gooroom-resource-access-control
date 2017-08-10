/*
 * grac_control.c
 *
 *  Created on: 2016. 6. 14.
 *      Author: yang
 */

/**
  @file 	 	grac_control.c
  @brief		구름 권한 관리 시스템에서 브라우저 실행을 제어한다..
  @details	class GracControl 구현 (GTK style의 c언어를 이용한 객체처리 사용) \n
  				어플리케이션은 grac_control 이외의 함수(class)는  사용하지 않는다.\n
  				헤더파일 :  	grac_control.h	\n
  				라이브러리:	libgrac.so
 */


#include "gac_control.h"
#include "grac_url_list.h"
#include "grm_log.h"
#include "cutility.h"
#include "sys_user.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

struct _GracControl {
	int		version1;

	char	*sender;
	char	*session;

	char	*error_msg;
	int	error_msg_size;	// buffer sieze, not string length

	int	retry_count;
	long	retry_delay;

	gboolean  d_opened;

	GracUrlList 	*url_list;

	int	version2;		// version1과 일치하여야함
};

static void adjust_url(char *src, char *dest, int size);

#define GRAC_CONTROL_VERSION 110

// 객체 주소가 올바른 정보를 가지고 있는지 확인한다.
static gboolean check_version(GracControl *ctrl, char *func)
{
	if (ctrl == NULL) {
		grm_log_error("%s : Invalid GracControl == NULL ????????", func);
		return FALSE;
	}
	if (ctrl->version1 != GRAC_CONTROL_VERSION) {
		grm_log_error("%s : Not support GracControl version (%d) : current : %s ????????",
									func, ctrl->version1, GRAC_CONTROL_VERSION);
		return FALSE;
	}
	if (ctrl->version1 != ctrl->version2) {
		grm_log_error("%s : Maybe broken GracControl ????????", func);
		return FALSE;
	}

	return TRUE;
}


static GracControl* _grac_control_alloc()
{
	GracControl *ctrl = malloc(sizeof(GracControl));

	if (ctrl) {
		memset(ctrl, 0, sizeof(GracControl));

		ctrl->version1 = GRAC_CONTROL_VERSION;
		ctrl->version2 = ctrl->version1;

		ctrl->retry_count = 10;
		ctrl->retry_delay = 10 * 1000;

		ctrl->url_list = grac_url_list_alloc();

		// 2017.2.6
/*
		if (grac_config_is_daemon_active(NULL, FALSE))
			ctrl->no_daemon_mode = FALSE;
		else
			ctrl->no_daemon_mode = TRUE;
*/
	}

	return ctrl;

}

/**
 @brief   GracControl 객체 생성
 @details
      객체 지향형 언어의 생성자에 해당한다.
 @param [in] sender		데몬에게 접속을 하는 주체를 구분하기 위한 것
 @param [in] session		접속후 해제까지 일련의 통신군을 하나의 그룹으로 구분하기 위한 것
 @return 생성된 객체 주소
 */
GracControl* grac_control_alloc(char *sender, char *session)
{
	GracControl *ctrl = _grac_control_alloc();

	if (ctrl) {
		if (sender)
			ctrl->sender = strdup(sender);
		if (session)
			ctrl->session = strdup(session);
	}

	return ctrl;

}

/**
@brief   GracControl 객체 해제
@details
      객체 지향형 언어의 소멸자에  해당한다. 단 자동으로 불려지는 것이 아니므로 필요한 시점에서 직접 호출하여야 한다.\n
      해제된  객체 주소값으로 0으로 설정하기 위해 이중 포인터를 사용한다.
@param [in,out]  pctrl  해제하고자 하는 객체 주소를 담는 변수의 주소 (double pointer)
 */
void	 grac_control_free(GracControl **pctrl)
{
	if (pctrl && *pctrl) {
		GracControl *ctrl = *pctrl;

		if (ctrl->sender)
			free(ctrl->sender);
		if (ctrl->session)
			free(ctrl->session);

		if (ctrl->error_msg)
			free(ctrl->error_msg);

		grac_url_list_free(&ctrl->url_list);

		free(ctrl);

		*pctrl = NULL;
	}

}

static void _set_error(GracControl *ctrl, char *function, char *errmsg)
{
	grm_log_error("%s() : %s", function, errmsg);

	if (ctrl && errmsg) {
		int n = strlen(errmsg) + 1;
		if (ctrl->error_msg_size < n) {
			char *ptr = malloc(n*2);
			if (ptr == NULL)
				return;
			if (ctrl->error_msg)
				free(ctrl->error_msg);
			ctrl->error_msg = ptr;
			ctrl->error_msg_size = n*2;
		}

		strcpy(ctrl->error_msg, errmsg);
	}
}

/**
  @brief   grac-daemon으로의 접속을 시도한다.
  @details
      grac_control 의 여러 기능을 사용하기 위해서는 반드시 open이 되어야 한다..
      단  daemon이 비활성화 모드로 설정되어 있는 경우는 데몬과의 접속없이 TRUE를 반환한다.
  @param [in]  ctrl     GracControl 객체주소
  @return gboolean 성공여부
 */
gboolean grac_control_open(GracControl *ctrl)
{
	gboolean ret = FALSE;

	if (ctrl) {
//		if (ctrl->no_daemon_mode == TRUE) {	//2017.2.4

		ctrl->d_opened = TRUE;
		ret = ctrl->d_opened;

		grac_url_list_load(ctrl->url_list, NULL, TRUE);

	}

	return ret;
}

/**
  @brief   grac-daemon으로의 접속을 끊는다.
  @details
      daemon이 비활성화 모드로 설정되어 있는 경우는 특별한 처리는 없다.
  @param [in]  ctrl     GracControl 객체주소
 */
void     grac_control_close(GracControl *ctrl)
{
	if (ctrl && ctrl->d_opened) {
//		if (ctrl->no_daemon_mode == FALSE) {	//2017.2.4

		ctrl->d_opened = FALSE;
	}
}


/**
  @brief   가장 최근의 오류 메시지를 구한다.
  @param [in]  ctrl  GracControl 객체주소
  @return char*		오류 메시지
 */
char* grac_control_get_error(GracControl *ctrl)
{
	char	*msg = "";

	if (ctrl && ctrl->error_msg)
		msg = ctrl->error_msg;

	return msg;

}

/**
  @brief   설정되어 있는 session명을 구한다.
  @param [in]  ctrl  GracControl 객체주소
  @return char*		session 명
 */
char* grac_control_get_session(GracControl* ctrl)
{
	if (ctrl)
		return ctrl->session;
	else
		return NULL;
}

static gboolean _get_login_name(char *name, int size)
{
	gboolean done;

	done = sys_user_get_login_name_by_api(name, size);
	if (done == FALSE)
		done = sys_user_get_login_name_by_wcmd(name, size);

	return done;
}


/**
  @brief   데몬에게 어플리케이션 실행을 요청한다.
  @details
  				어플리케이션에게 넘길 파라메터는 어플리케이션 다음에 나열하고 반드시 마지막은 NULL로 지정한다. \n
  				예) grac_control_app_executeL(ctrl, "www.naver.com",  "/usr/bin/browser", "http://www.naver.com", NULL); \n
  				어플리이케이션은 동일 권한인 경우는 동일 프로세스내에서 다른 권한일 때는 다른 프로세스에서 실행한다 \n
  				그 때 각 프로세스를 instance라고 칭하고 그 기준이 되는 값을 instance key라고 칭한다.
  @param [in]  ctrl  			GracControl 객체주소
  @param [in]  p_inst_key	instance key - 어플리케이션 권한 구분 키 (URL)
  @param [in]  path			application 경로명
  @return gboolean 성공여부
  */
gboolean grac_control_app_executeL(GracControl *ctrl, char *p_inst_key, char *path, ...)
{
	gboolean ret = TRUE;
	int		argc = 0;
	char	**argv;
	int		i, n;
	char	inst_key[256];
	char *func = "grac_control_app_executeL()";

	if (p_inst_key == NULL) {
		grm_log_debug("%s : invalid inst_key == NULL", func);
		return FALSE;
	}

	strncpy(inst_key, p_inst_key, sizeof(inst_key));

	check_version(ctrl, "grac_control_app_executeL()");

	grm_log_debug("%s : inst_key = %s", func, inst_key);
	grm_log_debug("%s : path = %s", func, path);

	va_list var_args;
	va_start (var_args, path);
	argc = 0;
	while (1) {
		char *p = va_arg(var_args, char*);
		if (p == NULL)
			break;
		grm_log_debug("%s : argv[%d] = %s", func, argc, p);
		argc++;
	}
	va_end (var_args);

	n = sizeof(char*) * (argc+1);
	argv = malloc(n);
	if (argv == NULL) {
		ret = FALSE;
	}
	else {
		memset(argv, 0, n);
		va_start (var_args, path);
		for (i=0; i<argc; i++) {
			char *p = va_arg(var_args, char*);
			if (p == NULL)
				break;
			argv[i] = strdup(p);
			if (argv[i] == NULL)
				ret = FALSE;
		}
		va_end (var_args);
	}

	if (ret == FALSE) {
		_set_error(ctrl, "grac_control_app_executeL", "out of memory");
	}
	else {
		ret = grac_control_app_executeV (ctrl, inst_key, path, argc, argv);
		if (ret == FALSE)
			_set_error(ctrl, "grac_control_app_executeL", "can;t execute");
	}

	if (argv) {
		for (i=0; i<argc; i++) {
			if (argv[i] != NULL)
				free(argv[i]);
		}
		free(argv);
	}

/*
	va_start (var_args, path);
	argc = 0;
	while (1) {
		char *p = va_arg(var_args, char*);
		if (p == NULL)
			break;
		argc++;
	}
	va_end (var_args);
*/

	grm_log_debug("end %s  : inst_key = %s", func, inst_key);

	return ret;
}


static gboolean get_browser_directory(gboolean allow, char *login, char *dir, int size)
{
	gboolean done = FALSE;

	if (c_strlen(login, 256) == 0)
		return FALSE;

	if (sys_user_get_home_dir_by_name(login, dir, size)) {
		if (allow)
			strncat (dir, "/trusted/", size-1);
		else
			strncat (dir, "/untrusted/", size-1);
		dir[size-1] = 0;

		done = TRUE;
		if (access(dir, F_OK) != 0) {
			if (mkdir(dir, 0755) < 0) {
				grm_log_error("mkdir : %s", strerror(errno));
				done = FALSE;
			}
		}
	}

	return done;
}

static gboolean _do_direct_execute(GracControl* ctrl, char *login_name, char *inst, char *path, int argc, char*argv[])
{
	gboolean done = FALSE;
	char	*func = "do_direct_execute()";
	gboolean allow;
	char **new_argv;
	int		i, n;
	char	dir[PATH_MAX+1];

	char	add_arg[PATH_MAX+20];

	allow = grac_url_list_is_allowed(ctrl->url_list, inst);

	done = get_browser_directory(allow, login_name, dir, sizeof(dir));
	if (done == FALSE) {
		grm_log_error("%s : can't make base directory for inst_key = %s", func, inst);
		return FALSE;
	}
	snprintf(add_arg, sizeof(add_arg), "--user-data-dir=%s", dir);

	n = sizeof(char*) * (argc+10);	// 주의: 10:이 함수에서 최대 9개까지만 추가
	new_argv = malloc(n);
	if (new_argv == NULL)
		return FALSE;
	memset(new_argv, 0, n);

	gboolean argv_url = FALSE;
	int idx = 0;
	new_argv[idx++] = path;
	for (i=0; i<argc; i++) {
		new_argv[idx++] = argv[i];
		if (argv[i][0] != '-')
			argv_url = TRUE;
	}
	new_argv[idx++] = add_arg;
	if (argv_url == FALSE) {
		if (c_strlen(inst, 256))
			new_argv[idx++] = inst;
	}

	int pid;
	pid = fork();
	if (pid < 0) {
		free(new_argv);
		return FALSE;
	}

	if (pid > 0) {
		free(new_argv);
		return TRUE;
	}


	grm_log_debug("new ----------------");
	grm_log_debug("%s  : actual - path = %s", func, path);
	for (i=0; i<idx; i++) {
		grm_log_debug("%s  : actual - argv[%d] = %s", func, i, new_argv[i]);
	}

	execv(path, new_argv);
	_exit(1);

	return FALSE;
}

/**
  @brief   데몬에게 어플리케이션 실행을 요청한다.
  @details
  				 grac_control_app_executeL()과 동일한 기능이나 파라메터를 배열로 넘기는 것에서 차이가 있다. \n
  				 파라메터 목록애 어플리케이션 경로는 포함하지 않는다.	\n
  				 리턴된 성공여부는 요청을 제대로 수신 했는지를 나타내며 어플리케이션이 실행되었는지를 뜻하지 않는다.
  @param [in]  ctrl  			GracControl 객체주소
  @param [in]  inst_key		instance key - 어플리케이션 권한 구분 키 (URL)
  @param [in]  path			application 경로명
  @param [in]  argc			파라메터 갯수
  @param [in]  argv			파라메터 리스트 (문자열 배열)
  @return gboolean 성공여부
  @todo
  				어플리케이션 실행 여부를 알려주는 기능 추가 필요
  */
gboolean grac_control_app_executeV(GracControl* ctrl, char *inst_key, char *path, int argc, char*argv[])
{
	gboolean ret = FALSE;
	char *func = "grac_control_app_executeV()";
	int i;

	check_version(ctrl, func);

	grm_log_debug("%s  : inst_key = %s", func, inst_key);
	grm_log_debug("%s  : path = %s", func, path);
	for (i=0; i<argc; i++) {
		grm_log_debug("%s  : argv[%d] = %s", func, i, argv[i]);
	}


	if (ctrl == NULL) {
		_set_error(ctrl, func, "invalid - ctrl is NULL");
		return FALSE;
	}

	if (ctrl->sender == NULL || *ctrl->sender == 0) {
		_set_error(ctrl, func, "invalid sender");
		return FALSE;
	}
	if (ctrl->session == NULL || *ctrl->session == 0) {
		_set_error(ctrl, func, "invalid session");
		return FALSE;
	}
/*
	if (inst_key == NULL || *inst_key == 0) {
		_set_error(ctrl, func, "invalid instance_key");
		return FALSE;
	}
*/
	if (path == NULL || *path == 0) {
		_set_error(ctrl, func, "invalid path");
		return FALSE;
	}

	char login_name[1024];
	if (_get_login_name(login_name, sizeof(login_name)) == FALSE) {
		_set_error(ctrl, func, "can't get login name");
		return FALSE;
	}

	char	adj_url[256];
	if (c_strlen(inst_key, 256))
		adjust_url(inst_key, adj_url, sizeof(adj_url));
	else
		adj_url[0] = 0;

	ret = _do_direct_execute(ctrl, login_name, inst_key, path, argc, argv);

	grm_log_debug("end %s  : inst_key = %s", func, inst_key);

	return ret;
}



static void adjust_url(char *src, char *dest, int size)
{
	char *ptr;

	ptr = strstr(src, "//");
	if (ptr)
		strncpy(dest, ptr+2, size);
	else
		strncpy(dest, src, size);

	ptr = strstr(dest, "/");
	if (ptr)
		*ptr = 0;

}

// Todo : from Daemon, URL 정규화
/**
  @brief   새로운 프로세스로 실행시켜야 하는지 확인한다.
  @details
  				 두 개의 instance key (URL)의 권한이 같은지를 확인한다. \n
  				 같으면 FALSE 다르면  TRUE를 리턴한다.
  @param [in]  ctrl  					GracControl 객체주소
  @param [in]  cur_instance  		기준이 되는 instance key
  @param [in]  new_instance		비교할 instance key
  @return gboolean 프로세스 변경 여부
  */

// 임시 ---------------------------------------------------------------------------------------------------------------
gboolean grac_control_app_change_instance(GracControl *ctrl, char *cur_instance, char *new_instance)
{
	gboolean need_change = FALSE;
	//char *func = "grac_control_app_change_instance()";

	if (ctrl && c_strlen(cur_instance, 256) && c_strlen(new_instance, 256)) {
		gboolean allow1, allow2;
		allow1 = grac_url_list_is_allowed(ctrl->url_list, cur_instance);
		allow2 = grac_url_list_is_allowed(ctrl->url_list, new_instance);

		if (allow1 != allow2)
			need_change = TRUE;
	}

	return need_change;
}

/**
  @brief   주어진 instance key(URL)의 권한 정보를 구한다.
  @param [in]  	ctrl  			GracControl 객체주소
  @param [in]	instance 	권한을 확인할 instance key (URL)
  @param [out]	info			권한 정보
  @return gboolean 성공 여부
  */

// 임시 ---------------------------------------------------------------------------------------------------------------
gboolean grac_control_app_get_instance_info(GracControl *ctrl, char *instance, GracInstanceInfo *info)
{
	gboolean done = FALSE;

	if (info)
		c_memset((char*)info, 0, sizeof(GracInstanceInfo));

	if (info) {
		info->acc_printer = 0;
		info->acc_network = 1;
		info->acc_usb     = 0;
		info->acc_capture = 0;
		info->acc_homedir = 1;
	}

	if (ctrl && ctrl->url_list ) {
		gboolean allow;
		allow = grac_url_list_is_allowed(ctrl->url_list, instance);
		if (allow) {
			if (info) {
				info->acc_printer = 1;
				info->acc_network = 1;
				info->acc_usb     = 1;
				info->acc_capture = 1;
				info->acc_homedir = 1;
			}
		}
		else {
			if (info) {
				info->acc_printer = 0;
				info->acc_network = 1;
				info->acc_usb     = 0;
				info->acc_capture = 0;
				info->acc_homedir = 1;
			}

		}

		done = TRUE;
	}
	return done;
}
