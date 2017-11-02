/*
 * sys_file.c
 *
 *  Created on: 2015. 11. 19.
 *      Author: user
 */

/**
  @file 	 	sys_file.c
  @brief		리눅스의 file system 관련 함수
  @details	헤더파일 :  	sys_file.h	\n
  				라이브러리:	libgrac.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include "sys_file.h"
#include "cutility.h"

/**
  @brief   사용자의 기본 홈 디렉토리 경로를 구한다.
  @details
  		등록되어 있지 않은 사용자의 경우는 /home/{사용자명}/ 으로 가정한다.
  @param [in]   user			사용자명
  @param [out] homeDir	홈디렉토리 경로
  @param [in]   size			homeDir 크기
  @return	gboolean	성공여부
 */
gboolean sys_file_get_default_home_dir(gchar *user, gchar *homeDir, int size)
{
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 buffer[2048];
	int    res;

	errno = 0;
	res = getpwnam_r(user, &get_pwd, buffer, sizeof(buffer), &res_pwd);
	if (res < 0 || res_pwd == NULL)
		return FALSE;

	if (get_pwd.pw_dir == NULL) {
		char	*base = "/home/";
		if (size < strlen(user) + strlen(base) + 2)
			return FALSE;
		sprintf(homeDir, "%s%s/", base, user);
	}
	else {
		if (size < strlen(get_pwd.pw_dir) + 2)
			return FALSE;
		strcpy(homeDir, get_pwd.pw_dir);
		char *ptr = strrchr(homeDir, '/');
		if (ptr == NULL || *(ptr+1) != 0)
			strcat(homeDir, "/");
	}

	return TRUE;
}

/**
  @brief   USB 삽입시의 기본 디렉토리 경로를 구한다.
  @details
  		/media/{사용자명}/ 으로 가정한다.
  @param [in]   user			사용자명
  @param [out] homeDir	디렉토리 경로
  @param [in]   size			homeDir 크기
  @return	gboolean	성공여부
 */
gboolean sys_file_get_default_usb_dir(gchar *user, gchar *homeDir, int size)
{
	char	*base = "/media/";

	if (size < strlen(user) + strlen(base) + 2)
		return FALSE;
	sprintf(homeDir, "%s%s/", base, user);
	return TRUE;
}

/**
  @brief   지정된 파일의 소유자와 그룹을 변경한다.
  @details
  		이 함수를 호출하는 프로세스는 변경할 수 있는 권힌을 가지고 있어야 한다.
  @param [in]   path			file or directory path
  @param [in] 	owner		owner name
  @param [in]   group		group name
  @return	gboolean	성공여부
 */
gboolean sys_file_change_owner_group(gchar *path, gchar* owner, gchar* group)
{
	gboolean done;

	char orgPwdStr[128];
	char orgGrpStr[128];
	int	 new_uid, new_gid;

	done = sys_file_get_owner_group(path,
																	orgPwdStr, sizeof(orgPwdStr),
																	orgGrpStr, sizeof(orgGrpStr));
	if (done == FALSE)
		return FALSE;

	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 pbuf[2048];
	struct group  get_grp;
	struct group  *res_grp;
	char	 gbuf[2048];
	int    res;

	errno = 0;
	if (c_strlen(owner, NAME_MAX) == 0) {
		owner = orgPwdStr;
		new_uid = -1;
	}
	else {
		res = getpwnam_r(owner, &get_pwd, pbuf, sizeof(pbuf), &res_pwd);
		if (res < 0 || res_pwd == NULL) {
			return FALSE;
		}
		new_uid = get_pwd.pw_uid;
	}

	if (c_strlen(group, NAME_MAX) == 0) {
		group = orgGrpStr;
		new_gid = -1;
	}
	else {
		res = getgrnam_r(group, &get_grp, gbuf, sizeof(gbuf), &res_grp);
		if (res < 0 || res_grp == NULL)
			return FALSE;
		new_gid = get_grp.gr_gid;
	}

	done = TRUE;
	if (c_strcmp(orgPwdStr, owner, NAME_MAX, -1) == 0) {
		if (c_strcmp(orgGrpStr, group, NAME_MAX, -1) == 0) {
			// no change
			done = TRUE;
		}
		else { // different group
			if (chown(path, -1, new_gid) < 0)
				done = FALSE;
		}
	}
	else {
		if (c_strcmp(orgGrpStr, group, NAME_MAX, -1) == 0) { // different owner
			if (chown(path, new_uid, -1) < 0)
				done = FALSE;
		}
		else { // different owner, group
			if (chown(path, new_uid, new_gid) < 0)
				done = FALSE;
		}
	}

	return done;
}

/**
  @brief   지정된 파일의 소유자와 그룹을 구한다.
  @param [in]    path			file or directory path
  @param [out]  owner		owner name
  @param [in] 	 oSize		buffer size of owner
  @param [out]  group		group name
  @param [in] 	 gSize		buffer size of group
  @return	gboolean	성공여부
 */
gboolean sys_file_get_owner_group(gchar *path, gchar *owner, int oSize,
		                                           gchar *group, int gSize)
{
	struct	stat finfo;

	if (stat(path, &finfo) < 0)
		return FALSE;

	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	char	 pbuf[2048];
	struct group  get_grp;
	struct group  *res_grp;
	char	 gbuf[2048];
	int    res;

	errno = 0;
	res = getpwuid_r(finfo.st_uid, &get_pwd, pbuf, sizeof(pbuf), &res_pwd);
	if (res < 0 || res_pwd == NULL)
		return FALSE;

	res = getgrgid_r(finfo.st_gid, &get_grp, gbuf, sizeof(gbuf), &res_grp);
	if (res < 0 || res_grp == NULL)
		return FALSE;

	if (oSize < strlen(get_pwd.pw_name)+1)
		return FALSE;

	if (gSize < strlen(get_grp.gr_name)+1)
		return FALSE;

	strcpy(owner, get_pwd.pw_name);
	strcpy(group, get_grp.gr_name);

	return TRUE;
}


static int g_mode_mask[9] = {
	S_IRUSR, S_IWUSR, S_IXUSR,
	S_IRGRP, S_IWGRP, S_IXGRP,
	S_IROTH, S_IWOTH, S_IXOTH
};
static char* g_mode_char = "rwxrwxrwx";

static gboolean _file_set_mode(gchar *path, char *modeStr, int off, int len)
{
	if (modeStr == NULL || strlen(modeStr) < len)
		return FALSE;

	int setMask = 0;
	int setVal = 0;
	int idx;
	for (idx=0; idx < len; idx++) {
		setMask |= g_mode_mask[idx+off];
		if (modeStr[idx] == g_mode_char[idx+off])
			setVal |= g_mode_mask[idx+off];
	}

	struct	stat finfo;

	if (stat(path, &finfo) < 0)
		return FALSE;

	mode_t mode = finfo.st_mode;
	mode &= ~setMask;
	mode |= (setVal & setMask);

	if (chmod(path, mode) < 0)
		return FALSE;
	else
		return TRUE;
}


/**
  @brief   파일 퍼미션을 설정한다.
  @details
  			user, group, other permission을 동시에 설정한다.
  @param [in]    path			file or directory path
  @param [in] 	 modeStr	문자열 형태의 퍼미션 (9자리) - 예: "rwxr-xr-x"
  @return	gboolean	성공여부
 */
gboolean sys_file_set_mode(gchar *path, gchar *modeStr)
{
	return _file_set_mode(path, modeStr, 0, 9);
}

/**
  @brief   파일의 사용자 퍼미션을 설정한다.
  @param [in]    path			file or directory path
  @param [in] 	 modeStr	문자열 형태의 퍼미션 (3자리) - 예: "rwx"
  @return	gboolean	성공여부
 */
gboolean sys_file_set_mode_user(gchar *path, gchar *modeStr)
{
	return _file_set_mode(path, modeStr, 0, 3);
}

/**
  @brief   파일의 그룹 퍼미션을 설정한다.
  @param [in]    path			file or directory path
  @param [in] 	 modeStr	문자열 형태의 퍼미션 (3자리) - 예: "rwx"
  @return	gboolean	성공여부
 */
gboolean sys_file_set_mode_group(gchar *path, gchar *modeStr)
{
	return _file_set_mode(path, modeStr, 3, 3);
}

/**
  @brief   파일의 other 퍼미션을 설정한다.
  @param [in]    path			file or directory path
  @param [in] 	 modeStr	문자열 형태의 퍼미션 (3자리) - 예: "rwx"
  @return	gboolean	성공여부
 */
gboolean sys_file_set_mode_other(gchar *path, gchar *modeStr)
{
	return _file_set_mode(path, modeStr, 6, 3);
}


// a size of modee size is 10 bytes at least
//gboolean sys_file_get_mode(gchar *path, gchar *mode);
// a size of modee size is 4 bytes at least
//gboolean sys_file_get_mode_user(gchar *path, gchar *mode);
// a size of modee size is 4 bytes at least
//gboolean sys_file_get_mode_group(gchar *path, gchar *mode);
// a size of modee size is 4 bytes at least
//gboolean sys_file_get_mode_other(gchar *path, gchar *mode);


static gboolean _do_make_directory(gchar *path, gchar* owner, gchar* group, int mode)
{
	gboolean done = TRUE;
	char *ptr;

	if (c_strlen(path, PATH_MAX) == 0)
		return TRUE;
	if (strcmp(path, "/") == 0)
		return TRUE;

	// already existing
	if (access(path, F_OK) == 0)
		return TRUE;

	ptr = strrchr(path, '/');
	if (ptr) {
		*ptr = 0;
		done = _do_make_directory(path, owner, group, mode);
		*ptr = '/';
		if (done == FALSE)
			return FALSE;
	}

	if (mkdir(path, mode) != 0) {
		done = FALSE;
	}
	else {
		if (c_strlen(owner, NAME_MAX) && c_strlen(group, NAME_MAX)) {
			done &= sys_file_change_owner_group(path, owner, group);
		}
		if (chmod(path, mode) != 0) {
			done = FALSE;
		}
	}

	return done;
}

/**
  @brief   지정된 소유자,그룹과 퍼미션을 가지는 새로운 디렉토리를 생성한다.
  @details
  			중간 계층의 디렉토리도 생성된다. \n
  			이미 존재하는 경우는  소유자,그룹과 퍼미션의 변화없이 TRUE로 반환된다.
  @param [in]    path			directory path
  @param [in] 	 owner		소유자
  @param [in] 	 group		그룹
  @param [in] 	 mode		permission
  @return	gboolean	성공여부
 */
gboolean sys_file_make_new_directory(gchar *path, gchar* owner, gchar* group, int mode)
{
	gboolean done = FALSE;

	if (c_strlen(path, PATH_MAX) > 0) {
		char *tmp;
		int size = c_strlen(path, PATH_MAX)+1;
		tmp = malloc(size);
		if (tmp) {
			int n;
			c_strcpy(tmp, path, size);
			n = c_strlen(tmp, PATH_MAX);
			if (tmp[n-1] == '/')
				tmp[n-1] = 0;
			done = _do_make_directory(tmp, owner, group, mode);
			free(tmp);
		}
	}

	return done;
}

int sys_file_get_length(gchar *path)
{
	int	length = -1;
	int	res;
	struct stat st;

	if (path) {
		res = stat(path, &st);
		if (res == 0)
			length = st.st_size;
	}

	return length;
}

gboolean sys_file_is_existing(gchar *path)
{
	gboolean exist = FALSE;

	if (path) {
		if (access(path, F_OK) == 0) {
			exist = TRUE;
		}
	}

	return exist;
}
