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
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <errno.h>
#include <ctype.h>

#define _GNU_SOURCE
#include <crypt.h>

#include "sys_user.h"
#include "sys_etc.h"
#include "cutility.h"
#include "grm_log.h"

static guint _get_id_range(char *key)
{
	//char *cmd = "grep ^%s /etc/login.defs | cut '-d ' -f2";
	char *cmd = "grep ^%s /etc/login.defs";
	char run[128], *ptr;
	FILE *fp;
	uint value = 0;

	sprintf(run, cmd, key);

	fp = popen(run, "r");
	if (fp != NULL) {
		char buf[1024];
	 	if (fgets(buf, sizeof(buf), fp) != NULL) {
	 		ptr = buf;
	 		while (*ptr) {
	 			if (*ptr == '#')
	 				break;
	 			if (*ptr >= '0' && *ptr <= '9')
	 				break;
	 			ptr++;
	 		}
	 		value = atoi(ptr);	// if invalid, return 0
	 	}
		pclose(fp);
	}

	return value;
}

/**
  @brief   리눅스에서 사용하는 일반사용자의 uid 범위를 구한다.
  @details
  			/etc/login.defs의 설정 내용을 이용하였다.
  @param [out] min	일반 사용자용 uid 최소값
  @param [out] max	일반 사용자용 uid 최대값
 */
void sys_user_get_uid_range(int* min, int *max)
{
	*min = _get_id_range("UID_MIN");
	if (*min == 0)
		*min = 1000;
	*max = _get_id_range("UID_MAX");
	if (*max == 0)
		*max = 60000;
}

/**
  @brief   리눅스에서 사용하는 일반그룹의 gid 범위를 구한다.
  @details
  			/etc/login.defs의 설정 내용을 이용하였다.
  @param [out] min	일반 그룹용 gid 최소값
  @param [out] max	일반 그룹용 gid 최대값
 */
void sys_user_get_gid_range(int* min, int *max)
{
	*min = _get_id_range("GID_MIN");
	if (*min == 0)
		*min = 1000;
	*max = _get_id_range("GID_MAX");
	if (*max == 0)
		*max = 60000;
}

/**
  @brief   리눅스에서 사용하는 시스템 사용자의 uid 범위를 구한다.
  @details
  			/etc/login.defs의 설정 내용을 이용하였다.
  @param [out] min	시스템용 사용자 uid 최소값
  @param [out] max	시스템용 사용자 uid 최대값
 */
void sys_user_get_sys_uid_range(int* min, int *max)
{
	*min = _get_id_range("SYS_UID_MIN");
	if (*min == 0) {
		*min = 1;
	}

	*max = _get_id_range("SYS_UID_MAX");
	if (*max == 0) {
		int umin, umax;
		sys_user_get_uid_range(&umin, &umax);
		*max = umin - 1;
	}
}

/**
  @brief   리눅스에서 사용하는 시스템 그룹의 gid 범위를 구한다.
  @details
  			/etc/login.defs의 설정 내용을 이용하였다.
  @param [out] min	시스템용 그룹 gid 최소값
  @param [out] max	시스템용 그룹 gid 최대값
 */
void sys_user_get_sys_gid_range(int* min, int *max)
{
	*min = _get_id_range("SYS_GID_MIN");
	if (*min == 0) {
		*min = 1;
	}

	*max = _get_id_range("SYS_GID_MAX");
	if (*max == 0) {
		int gmin, gmax;
		sys_user_get_gid_range(&gmin, &gmax);
		*max = gmin - 1;
	}
}


/**
  @brief   지정된 사용자의 이름의 uid를 구한다.
  @param [in] name	사용자명
  @return	int	사용자 uid,  오류시는 -1

 */
int sys_user_get_uid_from_name(gchar* name)
{
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 buffer[2048];
	int    res;

	errno = 0;
	res = getpwnam_r(name, &get_pwd, buffer, sizeof(buffer), &res_pwd);
	if (res < 0)
		return -1;

	if (res_pwd == NULL)
		return -1;

	return (int)get_pwd.pw_uid;
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
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 buffer[2048];
	int    res;

	errno = 0;
	res = getpwuid_r(uid, &get_pwd, buffer, sizeof(buffer), &res_pwd);
	if (res < 0)
		return FALSE;

	if (res_pwd == NULL)
		return FALSE;

	strncpy(name, get_pwd.pw_name, maxlen);
	return TRUE;
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
	char	 buffer[2048];
	int    res;

	errno = 0;
	res = getgrnam_r(name, &get_grp, buffer, sizeof(buffer), &res_grp);
	if (res < 0)
		return -1;

	if (res_grp == NULL)
		return -1;

	return get_grp.gr_gid;
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
	struct group  get_grp;
	struct group  *res_grp;
	char	 buffer[2048];
	int    res;

	errno = 0;
	res = getgrgid_r(gid, &get_grp, buffer, sizeof(buffer), &res_grp);
	if (res < 0)
		return FALSE;

	if (res_grp == NULL)
		return FALSE;

	strncpy(name, get_grp.gr_name, maxlen);
	return TRUE;
}


/**
  @brief   사용자가 지정된 그룹에 속해 있는지 확인한다.
  @details
  		 group은 primary 그룹과 보조 그룹 모두를 대상으로 한다.
  @param [in]   user		사용자
  @param [in]   group	그룹명
  @return	gboolean	소속 여부
 */
gboolean sys_user_is_in_group(gchar* user, gchar* group)
{
	gboolean done = FALSE;

	// check main group
	char	user_main_group[2048];
	if (sys_user_get_main_group_from_name(user, user_main_group, sizeof(user_main_group)) ) {
		if (strcmp(user_main_group, group) == 0)
			return TRUE;
	}

	struct group  get_grp;
	struct group  *res_grp;
	char	 buffer[2048];
	int    res, i;

	// check supplementary group
	errno = 0;
	res = getgrnam_r(group, &get_grp, buffer, sizeof(buffer), &res_grp);
	if (res == 0 && res_grp != NULL && get_grp.gr_mem != NULL) {
			for (i = 0; get_grp.gr_mem[i] != NULL; i++) {
				if (strcmp(user, get_grp.gr_mem[i]) == 0) {
					done = TRUE;
					break;
				}
			}
	 }

	return done;
}

/**
  @brief   지정된 그룹을 메인 그룹으로 가지고 있는 사용자를 구한다.
  @details
  		 group은 primary 그룹만이 대상이 된다
  @param [in]   group	그룹명
  @return	gboolean	소속 여부
  @warning 복수 개 존재시의 처리 미비
 */
int	sys_user_get_main_uid (gchar *group)
{
	int gid;
	int uid = -1;

	gid = sys_user_get_gid_from_name(group);
	if (gid == -1)
		return -1;

	struct passwd *pwd;
	setpwent();
	pwd = getpwent();
	while (pwd) {
		if (pwd->pw_gid == gid) {
			uid = pwd->pw_uid;
			break;
		}
		pwd = getpwent();
	}
	endpwent();

	return uid;
}

/**
  @brief   지정된 사용자의 메인 그룹을 구한다.
  @param [in]   uid		사용자ID
  @param [out] group	그룹명
  @param [in]   maxlen	버퍼크기
  @return	gboolean	성공 여부
 */
gboolean sys_user_get_main_group_from_uid(int uid, gchar *group, int maxlen)
{
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 buffer[2048];
	int    res;

	errno = 0;
	res = getpwuid_r(uid, &get_pwd, buffer, sizeof(buffer), &res_pwd);
	if (res == 0 && res_pwd != NULL) {
		return sys_user_get_name_from_gid(get_pwd.pw_gid, group, maxlen);
	}
	else {
		*group = 0;
		return FALSE;
	}
}

/**
  @brief   지정된 사용자의 메인 그룹을 구한다.
  @param [in]   name	사용자명
  @param [out] group	그룹명
  @param [in]   maxlen	버퍼크기
  @return	gboolean	성공 여부
 */
gboolean sys_user_get_main_group_from_name(gchar *name, gchar *group, int maxlen)
{
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 buffer[2048];
	int    res;

	errno = 0;
	res = getpwnam_r(name, &get_pwd, buffer, sizeof(buffer), &res_pwd);
	if (res == 0 && res_pwd != NULL) {
		return sys_user_get_name_from_gid(get_pwd.pw_gid, group, maxlen);
	}
	else {
		*group = 0;
		return FALSE;
	}
}

/**
  @brief   지정된 사용자의 홈 디렉토리가 존재하는지 확인한다.
  @details
  			고정된 디렉토리 명으로 처리하지 않고 시스템에 저장되어 있는 사용자 정보를 이용하여 확인한다.
  @param [in]   uid	사용자ID
  @return	gboolean	존재 여부
 */
gboolean sys_user_check_home_dir_by_uid(int uid)
{
	gboolean check = FALSE;
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 buffer[2048];
	int    res;

	errno = 0;
	res = getpwuid_r(uid, &get_pwd, buffer, sizeof(buffer), &res_pwd);
	if (res == 0 && res_pwd != NULL) {
		if (access(get_pwd.pw_dir, F_OK) == 0) {
			check = TRUE;
		}
	}
	return check;
}

/**
  @brief   지정된 사용자의 홈 디렉토리가 존재하는지 확인한다.
  @details
  			고정된 디렉토리 명으로 처리하지 않고 시스템에 저장되어 있는 사용자 정보를 이용하여 확인한다.
  @param [in]   name 사용자명
  @return	gboolean	존재 여부
 */
gboolean sys_user_check_home_dir_by_name(char *name)
{
	gboolean check = FALSE;
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 buffer[2048];
	int    res;

	errno = 0;
	res = getpwnam_r(name, &get_pwd, buffer, sizeof(buffer), &res_pwd);
	if (res == 0 && res_pwd != NULL) {
		if (access(get_pwd.pw_dir, F_OK) == 0) {
			check = TRUE;
		}
	}
	return check;
}

gboolean sys_user_get_home_dir_by_uid(int uid, char *dir, int size)
{
	gboolean done = FALSE;
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 buffer[2048];
	int    res;

	errno = 0;
	res = getpwuid_r(uid, &get_pwd, buffer, sizeof(buffer), &res_pwd);
	if (res == 0 && res_pwd != NULL) {
		c_strcpy(dir, get_pwd.pw_dir, size);
		done = TRUE;
	}
	return done;
}

gboolean sys_user_get_home_dir_by_name(char *name, char *dir, int size)
{
	gboolean done = FALSE;
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 buffer[2048];
	int    res;

	errno = 0;
	res = getpwnam_r(name, &get_pwd, buffer, sizeof(buffer), &res_pwd);
	if (res == 0 && res_pwd != NULL) {
		c_strcpy(dir, get_pwd.pw_dir, size);
		done = TRUE;
	}
	return done;
}


/**
  @brief   지정된 사용자와 패스워드로 로그인이 가능한지 확인한다.
  @details
  			패스워드가 문자열로 표현되므로 사용에 주의한다.
  @param [in]   s_user 사용자명
  @param [in]   s_pwd 비밀번호
  @return	gboolean	로구인 성공 여부
 */
/*
gboolean sys_check_login(char *s_user, char *s_pwd)
{
	gboolean check = FALSE;

	if (s_user == NULL || s_pwd == NULL)
		return FALSE;

	struct passwd *pwd;
	struct spwd  *spwd;
	char	*pw_passwd;

	pwd = getpwnam(s_user);
	if (pwd) {
		spwd = getspnam(s_user);
		if (spwd == NULL && errno == EACCES) {
			// grm_log_error("sys_check_login() : %s", strerror(errno));
			printf("sys_check_login() : %s\n", strerror(errno));
			return FALSE;
		}
		if (spwd != NULL)
			pw_passwd = spwd->sp_pwdp;
		else
			pw_passwd = pwd->pw_passwd;

		char *encrypt;

		encrypt = crypt(s_pwd, pw_passwd);
		// clear org to secure conetnes
		// char *p;
		// for (p=pwd; *p; ) *p++ = 0;
		if (encrypt) {
			if (strcmp(encrypt, pw_passwd) == 0)
				check = TRUE;
			else
			 printf("sys_check_login() : not same pwd\n");
		}
		else {
		 printf("sys_check_login() : crypt error %s\n", strerror(errno));
	    }
	}
	else {
	 printf("sys_check_login() : nt existing user\n");
	}

	return check;
}
*/

/**
  @brief   현재 로그인 된 사용자의 이름을 구한다.
  @details
  			로그인 하지않은 상태에서 실행하는 데몬등에서는 호출한 경우에는 제대로 구해지지 않음으로 주의한다.
  @param [out] name	로그인 사용자명
  @param [in]    size		버퍼 길이
 */
gboolean sys_user_get_login_name_by_api(char *name, int size)
{
	int n = getlogin_r(name, size);
	if (n == 0)
		return TRUE;
	else
		return FALSE;
}

// notice! result is only 8 characters
gboolean sys_user_get_login_name_by_wcmd(char *name, int size)
{
	char	*func = (char*)__FUNCTION__;
	FILE	*fp;
	char  *cmd = "w -h -s";

	fp = popen(cmd, "r");
	if (fp == NULL) {
			grm_log_error("%s : can't run - %s", func, cmd);
			return FALSE;
	}

	int fd = fileno(fp);
	if (fd == -1) {
		grm_log_error("%s : fileno() : %s", func, strerror(errno));
		pclose(fp);
		return FALSE;
	}
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		grm_log_error("%s : fcntl() : not set NONBLOCK: %s", func, strerror(errno));
		pclose(fp);
		return FALSE;
	}

	char buf[1024];
	int idx = 0, cnt, res;
	int nodata = 0;
	gboolean stop = FALSE;
	gboolean getB = FALSE;

	while (!stop) {
		cnt = sizeof(buf) - idx;
		res = read(fd, &buf[idx], cnt-1);
		if (res == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				nodata++;
				usleep(1000);
				if (nodata > 1000) {  // about one second
					//grm_log_debug("%s : no data : stop", func);
					break;
				}
			}
			else {
				//grm_log_debug("%s : error : %s", func, strerror(errno));
				break;
			}
		}
		else if (res == 0) {  // closed
			break;
		}
		else {
			nodata = 0;

			int i;
			idx += res;
			buf[idx] = 0;

					for (i=0; i<idx; i++) {
							if (buf[i] == 0x0d || buf[i] == 0x0a) {
								buf[i] = 0;

								// check
								char *p;
								p = strstr(buf, ":0");
								if (p && p[2] <= 0x20) {
									int r_idx = 0;
									while (r_idx < size-1) {
										name[r_idx] = buf[r_idx];
										if (buf[r_idx] <= 0x20)
											break;
										r_idx++;
									}
									name[r_idx] = 0;
									if (r_idx > 0) {
										stop = TRUE;
										getB = TRUE;
										break;
									}
								}

								i++;
								while (buf[i] == 0x0d || buf[i] == 0x0a)
									i++;

								memmove(buf, &buf[i], idx-i);
								idx -= i;
								buf[idx] = 0;
								i = -1;
							}
					}
		}
	} // while

	if (getB) {
		grm_log_debug("%s : get login name : %s", func, name);
	}

	pclose(fp);

	return getB;
}

gboolean sys_user_get_login_name_by_who_cmd(char *name, int size)
{
	gboolean done;
	char	output[2048];
	char	*cmd;

	cmd = "who | awk '{print $1}'";
	done =  sys_run_cmd_get_output(cmd, "by_who_cmd", output, sizeof(output));
	if (done == FALSE) {
		grm_log_error("commamd running error : %s", cmd);
		return FALSE;
	}

	int idx;
	for (idx=0; idx < sizeof(output); idx++) {
		if (output[idx] == 0)
			break;
		if (output[idx] == '\n') {
			output[idx] = 0;
			break;
		}
	}

	c_strtrim(output, sizeof(output));
	c_strcpy(name, output, size);

	return done;
}

gboolean sys_user_get_login_name_by_users_cmd(char *name, int size)
{
	gboolean done;
	char	output[2048];
	char	*cmd;

	cmd = "users";
	done =  sys_run_cmd_get_output(cmd, "by_users_cmd", output, sizeof(output));
	if (done == FALSE) {
		grm_log_error("commamd running error : %s", cmd);
		return FALSE;
	}

	int idx;
	for (idx=0; idx < sizeof(output); idx++) {
		if (output[idx] == 0)
			break;
		if (output[idx] == '\n') {
			output[idx] = 0;
			break;
		}
	}

	c_strtrim(output, sizeof(output));
	c_strcpy(name, output, size);

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
	char	user[1024];

	if (sys_user_get_login_name(user, sizeof(user)) ) {
		uid = sys_user_get_uid_from_name(user);
		if (uid < 0)
			grm_log_debug("%s : login name : %d %d", __FUNCTION__, user, uid);
	}
	else {
		grm_log_debug("%s : can't get login name", __FUNCTION__);
	}

	return uid;
}
