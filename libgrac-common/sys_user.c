/*
 * sys_user.c
 *
 *  Created on: 2015. 11. 19.
 *      Author: user
 */

/**
  @file 	 	sys_user.c
  @brief		리눅스 시스템의 사용자에 관련된 함수
  @details	시스템 혹은 특정 라이브러리 의존적인 부분을 편리하게 사용하기 위한 함수들로 구성되어 있다.\n
  				헤더파일 :  	sys_user.h	\n
  				라이브러리:	libgrac.so
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <errno.h>
#include <ctype.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "sys_user.h"
#include "sys_etc.h"
#include "cutility.h"
#include "grac_log.h"

/**
  @brief   지정된 사용자의 이름의 uid를 구한다.
  @param [in] name	사용자명
  @return	int	사용자 uid,  오류시는 -1

 */
int sys_user_get_uid_from_name(gchar* name)
{
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 *buf;
	int		 buf_size = 2048;
	int    res = -1;

	buf = c_alloc(buf_size);

	if (buf) {
		errno = 0;
		res = getpwnam_r(name, &get_pwd, buf, buf_size, &res_pwd);
		if (res == 0) {
			if (res_pwd == NULL)
				res = -1;
			else
				res = (int)get_pwd.pw_uid;
		}
		c_free(&buf);
	}

	return res;
}

/**
  @brief   지정된 uid의 사용자의 이름을 구한다.
  @param [in]   uid		사용자 ID
  @param [out] name	사용자명
  @param [in]   maxlen	버퍼 길이
  @return	gboolean	성공여부
 */
gboolean sys_user_get_name_from_uid(int uid, gchar *name, int maxlen)
{
	gboolean done = FALSE;
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 *buf;
	int		 buf_size = 2048;
	int    res = -1;

	buf = c_alloc(buf_size);

	if (buf) {
		errno = 0;
		res = getpwuid_r(uid, &get_pwd, buf, buf_size, &res_pwd);
		if (res == 0) {
			if (res_pwd != NULL) {
				c_strcpy(name, get_pwd.pw_name, maxlen);
				done = TRUE;
			}
		}
		c_free(&buf);
	}

	return done;
}

/**
  @brief   지정된 그룹 이름의 gid를 구한다.
  @param [in] name	그룹명
  @return	int	그룹 gid,  오류시는 -1
 */
int sys_user_get_gid_from_name(gchar* name)
{
	struct group  get_grp;
	struct group  *res_grp;
	char	 *buf;
	int		 buf_size = 2048;
	int    res = -1;

	buf = c_alloc(buf_size);

	if (buf) {
		errno = 0;
		res = getgrnam_r(name, &get_grp, buf, buf_size, &res_grp);
		if (res == 0) {
			if (res_grp == NULL)
				res = -1;
			else
				res = get_grp.gr_gid;
		}
		c_free(&buf);
	}

	return res;
}

/**
  @brief   지정된 gid의 그룹 이름을 구한다.
  @param [in]   gid		그룹 ID
  @param [out] name	그룹명
  @param [in]   maxlen	버퍼 길이
  @return	gboolean	성공여부
 */
gboolean sys_user_get_name_from_gid(int gid, gchar *name, int maxlen)
{
	gboolean done = FALSE;
	struct group  get_grp;
	struct group  *res_grp;
	char	 *buf;
	int		 buf_size = 2048;
	int    res = -1;

	buf = c_alloc(buf_size);

	if (buf) {
		errno = 0;
		res = getgrgid_r(gid, &get_grp, buf, buf_size, &res_grp);
		if (res == 0) {
			if (res_grp != NULL) {
				c_strcpy(name, get_grp.gr_name, maxlen);
				res = TRUE;
			}
		}
		c_free(&buf);
	}
	return done;
}



/**
  @brief   현재 로그인 된 사용자의 이름을 구한다.
  @details
  			로그인 하지않은 상태에서 실행하는 데몬등에서는 호출한 경우에는 제대로 구해지지 않음으로 주의한다.
  @param [out] name	로그인 사용자명
  @param [in]    size		버퍼 길이
 */
static gboolean sys_user_get_login_name_by_api(char *name, int size)
{
	int n = getlogin_r(name, size);
	if (n == 0)
		return TRUE;
	else
		return FALSE;
}

static gboolean sys_user_get_login_name_by_who_cmd(char *name, int size)
{
	gboolean done = FALSE;
	char	*buffer;
	int		buffer_size = 2048;
	char	*cmd;

	buffer = c_alloc(buffer_size);
	if (buffer == NULL)
		return FALSE;

	cmd = "who | awk '{print $1}'";
	done =  sys_run_cmd_get_output(cmd, "by_who_cmd", buffer, buffer_size);
	if (done == FALSE) {
		grac_log_error("commamd running error : %s", cmd);
		goto finish;
	}

	int idx;
	for (idx=0; idx < buffer_size; idx++) {
		if (buffer[idx] == 0)
			break;
		if (buffer[idx] == '\n') {
			buffer[idx] = 0;
			break;
		}
	}

	c_strtrim(buffer, buffer_size);
	c_strcpy(name, buffer, size);
	if (c_strlen(name, size) <= 0) {
		grac_log_error("empty data : %s", cmd);
		done = FALSE;
	}

finish:
	c_free(&buffer);

	return done;
}

static gboolean sys_user_get_login_name_by_users_cmd(char *name, int size)
{
	gboolean done = FALSE;
	char	*buffer;
	int		buffer_size = 2048;
	char	*cmd;

	buffer = c_alloc(buffer_size);
	if (buffer == NULL)
		return FALSE;

	cmd = "users";
	done =  sys_run_cmd_get_output(cmd, "by_users_cmd", buffer, buffer_size);
	if (done == FALSE) {
		grac_log_error("commamd running error : %s", cmd);
		goto finish;
	}

	int idx;
	for (idx=0; idx < buffer_size; idx++) {
		if (buffer[idx] == 0)
			break;
		if (buffer[idx] == '\n') {
			buffer[idx] = 0;
			break;
		}
	}

	c_strtrim(buffer, buffer_size);
	c_strcpy(name, buffer, size);

finish:
	c_free(&buffer);

	return done;
}

gboolean sys_user_get_login_name(char *name, int size)
{
	gboolean done;

	done = sys_user_get_login_name_by_api(name, size);
	if (done == FALSE)
		done = sys_user_get_login_name_by_who_cmd(name, size);
	if (done == FALSE)
		done = sys_user_get_login_name_by_users_cmd(name, size);

	return done;
}

int sys_user_get_login_uid()
{
	int 	uid = -1;
	char	*user;
	int		user_size = NAME_MAX;

	user = c_alloc(user_size);
	if (user) {
		if (sys_user_get_login_name(user, user_size) ) {
			uid = sys_user_get_uid_from_name(user);
			if (uid < 0)
				grac_log_debug("%s : login name : %d %d", __FUNCTION__, user, uid);
		}
		else {
			grac_log_debug("%s : can't get login name", __FUNCTION__);
		}
		c_free(&user);
	}

	return uid;
}
