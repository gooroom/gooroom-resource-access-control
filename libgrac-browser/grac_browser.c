/*
 * grac_browser.c
 *
 *  Created on: 2017. 7. 3.
 *      Author: yang
 */

/**
  @file 	 	grac_browser.c
  @brief		브라우저 실행을 제어한다..
  @details	class GracBrowser 구현 (GTK style의 c언어를 이용한 객체처리 사용) \n
  				헤더파일 :  	grac_browser.h	\n
  				라이브러리:	libgrac-browser.so
 */


#include "grac_browser.h"
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
#include <sys/stat.h>
#include <sys/types.h>

struct _GracBrowser {
	int		version1;

	GracUrlList 	*url_list;

	int	version2;		// version1과 일치하여야함
};

#define GRAC_BROWSER_VERSION 130

// 객체 주소가 올바른 정보를 가지고 있는지 확인한다.
static gboolean check_version(GracBrowser *ctrl, char *func)
{
	if (ctrl == NULL) {
		grm_log_error("%s : Invalid GracBrowser == NULL ", func);
		return FALSE;
	}
	if (ctrl->version1 != GRAC_BROWSER_VERSION) {
		grm_log_error("%s : Not support GracBrowser version (%d) : current : %s ",
									func, ctrl->version1, GRAC_BROWSER_VERSION);
		return FALSE;
	}
	if (ctrl->version1 != ctrl->version2) {
		grm_log_error("%s : Maybe broken GracBrowser ", func);
		return FALSE;
	}

	return TRUE;
}

/**
 @brief   GracBrowser 객체 생성
 @details
      객체 지향형 언어의 생성자에 해당한다.
 @return 생성된 객체 주소
 */
GracBrowser* grac_browser_alloc()
{
	GracBrowser *ctrl = malloc(sizeof(GracBrowser));

	if (ctrl) {
		memset(ctrl, 0, sizeof(GracBrowser));

		ctrl->version1 = GRAC_BROWSER_VERSION;
		ctrl->version2 = ctrl->version1;

		ctrl->url_list = grac_url_list_alloc();

	}

	return ctrl;
}

/**
@brief   GracBrowser 객체 해제
@details
      객체 지향형 언어의 소멸자에  해당한다. 단 자동으로 불려지는 것이 아니므로 필요한 시점에서 직접 호출하여야 한다.\n
      해제된  객체 주소값으로 0으로 설정하기 위해 이중 포인터를 사용한다.
@param [in,out]  pctrl  해제하고자 하는 객체 주소를 담는 변수의 주소 (double pointer)
 */
void	 grac_browser_free(GracBrowser **pctrl)
{
	if (pctrl && *pctrl) {
		GracBrowser *ctrl = *pctrl;

		grac_url_list_free(&ctrl->url_list);

		free(ctrl);

		*pctrl = NULL;
	}

}

static gboolean _get_login_name(char *name, int size)
{
	gboolean done;

	done = sys_user_get_login_name_by_api(name, size);
	if (done == FALSE)
		done = sys_user_get_login_name_by_wcmd(name, size);

	return done;
}

/*
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
*/

static gboolean _get_browser_directory(gboolean allow, char *login, char *dir, int size)
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

static gboolean _do_direct_execute(GracBrowser* ctrl, char *url, char *path, int argc, char*argv[])
{
	gboolean done = FALSE;
	char	*func = "do_direct_execute()";
	gboolean allow;
	char **new_argv;
	int		i, n;
	char	dir[PATH_MAX+1];
	char  login_name[1024];

	if (_get_login_name(login_name, sizeof(login_name)) == FALSE) {
		grm_log_error("%s : can't get login name", func);
		return FALSE;
	}

	allow = grac_url_list_is_allowed(ctrl->url_list, url);

	done = _get_browser_directory(allow, login_name, dir, sizeof(dir));
	if (done == FALSE) {
		grm_log_error("%s : can't make base directory for url = %s", func, url);
		return FALSE;
	}

	char	arg_datadir[PATH_MAX+20];
	snprintf(arg_datadir, sizeof(arg_datadir), "--user-data-dir=%s", dir);

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
	new_argv[idx++] = arg_datadir;
	if (argv_url == FALSE) {
		if (c_strlen(url, grac_config_max_length_url()) > 0)
			new_argv[idx++] = url;
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
  				어플리케이션에게 넘길 파라메터는 어플리케이션 다음에 나열하고 반드시 마지막은 NULL로 지정한다. \n
  				예) grac_browser_app_executeL(ctrl, "www.naver.com",  "/usr/bin/browser", NULL); \n
  				어플리이케이션은 동일 권한인 경우는 동일 프로세스내에서 다른 권한일 때는 다른 프로세스에서 실행한다 \n
  				그 때 각 프로세스를 instance라고 칭하고 그 기준이 되는 값을 instance key라고 칭한다.
  @param [in]  ctrl  			GracBrowser 객체주소
  @param [in]  p_inst_key	instance key - 어플리케이션 권한 구분 키 (URL)
  @param [in]  path			application 경로명
  @return gboolean 성공여부
  */
gboolean grac_browser_executeL(GracBrowser *ctrl, char *url, char *path, ...)
{
	gboolean ret = TRUE;
	int		argc = 0;
	char	**argv;
	int		i, n;
	char	*copy_url;
	char  *func = "grac_browser_executeL()";

	if (ctrl == NULL) {
		grm_log_error("%s : invalid - ctrl is NULL", func);
		return FALSE;
	}
	if (c_strlen(url, grac_config_max_length_url()) <= 0) {
		grm_log_debug("%s : invalid URL == NULL", func);
		return FALSE;
	}
	if (c_strlen(path, grac_config_max_length_path()) <= 0) {
		grm_log_error("%s : invalid path", func);
		return FALSE;
	}

	check_version(ctrl, func);

	copy_url = strndup(url, grac_config_max_length_url());
	if (copy_url == NULL) {
		grm_log_debug("%s : out of memory", func);
		return FALSE;
	}

	grm_log_debug("%s : url  = %s", func, copy_url);
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
		grm_log_error("grac_browser_app_executeL() : out of memory");
	}
	else {
		ret = _do_direct_execute(ctrl, url, path, argc, argv);
		if (ret == FALSE)
			grm_log_error("grac_browser_app_executeL() : can't execute");
	}

	if (argv) {
		for (i=0; i<argc; i++) {
			if (argv[i] != NULL)
				free(argv[i]);
		}
		free(argv);
	}

	free(copy_url);

//	grm_log_debug("end %s  : url = %s", func, copy_url);

	return ret;
}

/**
  @brief   데몬에게 어플리케이션 실행을 요청한다.
  @details
  				 grac_browser_app_executeL()과 동일한 기능이나 파라메터를 배열로 넘기는 것에서 차이가 있다. \n
  				 파라메터 목록애 어플리케이션 경로는 포함하지 않는다.	\n
  				 리턴된 성공여부는 요청을 제대로 수신 했는지를 나타내며 어플리케이션이 실행되었는지를 뜻하지 않는다.
  @param [in]  ctrl  			GracBrowser 객체주소
  @param [in]  inst_key		instance key - 어플리케이션 권한 구분 키 (URL)
  @param [in]  path			application 경로명
  @param [in]  argc			파라메터 갯수
  @param [in]  argv			파라메터 리스트 (문자열 배열)
  @return gboolean 성공여부
  @todo
  				어플리케이션 실행 여부를 알려주는 기능 추가 필요
  */
gboolean grac_browser_executeV(GracBrowser* ctrl, char *url, char *path, int argc, char*argv[])
{
	gboolean ret = FALSE;
	char *func = "grac_browser_app_executeV()";
	char *copy_url;

	if (ctrl == NULL) {
		grm_log_error("%s : invalid - ctrl is NULL", func);
		return FALSE;
	}
	if (c_strlen(url, grac_config_max_length_url()) <= 0) {
		grm_log_error("%s : invalid URL", func);
		return FALSE;
	}
	if (c_strlen(path, grac_config_max_length_path()) <= 0) {
		grm_log_error("%s : invalid path", func);
		return FALSE;
	}

	check_version(ctrl, func);

	copy_url = strndup(url, grac_config_max_length_url());
	if (copy_url == NULL) {
		grm_log_debug("%s : out of memory", func);
		return FALSE;
	}

	grm_log_debug("%s : url  = %s", func, copy_url);
	grm_log_debug("%s : path = %s", func, path);

	ret = _do_direct_execute(ctrl, copy_url, path, argc, argv);

	free(copy_url);

	grm_log_debug("end %s  : inst_key = %s", func, copy_url);

	return ret;
}


gboolean grac_browser_exec(GracBrowser *ctrl, char *url)
{
	return grac_browser_executeL(ctrl, url, (char*)grac_config_path_browser_app(), NULL);
}

/**
  @brief   주어진 URL의 신뢰 여부를 확인한다.
  @details
  				신뢰목록(trusted)에 없으면 비신뢰로 간주한다.
  @param [in]  	ctrl  	GracBrowser 객체주소
  @param [in]	  url    	신뢰 여부를 확인할 URL
  @return gboolean 신뢰  여부
  */

gboolean grac_browser_is_trusted(GracBrowser *ctrl, char *url)
{
	return grac_url_list_is_allowed(ctrl->url_list, url);
}
