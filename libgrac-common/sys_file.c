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
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>

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
gboolean sys_file_get_default_home_dir(gchar *user, gchar *home_dir, int home_dir_size)
{
	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	int		 buffer_size = PATH_MAX;
	char	 *buffer;
	int    res;

	buffer = c_alloc(buffer_size+1);
	if (buffer == NULL)
		return FALSE;

	errno = 0;
	res = getpwnam_r(user, &get_pwd, buffer, buffer_size, &res_pwd);
	if (res < 0 || res_pwd == NULL) {
		c_free(&buffer);
		return FALSE;
	}

	if (get_pwd.pw_dir == NULL) {
		char	*base = "/home/";
		if (home_dir_size < c_strlen(user, NAME_MAX) + c_strlen(base, PATH_MAX) + 2) {
			c_free(&buffer);
			return FALSE;
		}
		g_snprintf(home_dir, home_dir_size, "%s%s/", base, user);
	}
	else {
		if (home_dir_size < c_strlen(get_pwd.pw_dir, PATH_MAX) + 2) {
			c_free(&buffer);
			return FALSE;
		}
		c_strcpy(home_dir, get_pwd.pw_dir, home_dir_size);
		char *ptr = c_strrchr(home_dir, '/', home_dir_size);
		if (ptr == NULL || *(ptr+1) != 0)
			c_strcat(home_dir, "/", home_dir_size);
	}

	c_free(&buffer);

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

	if (size < c_strlen(user, NAME_MAX) + c_strlen(base, PATH_MAX) + 2)
		return FALSE;

	g_snprintf(homeDir, size, "%s%s/", base, user);

	return TRUE;
}

static int s_chown(char *path, uid_t uid, gid_t gid)
{
	int	res = -1;
	int fd;

	fd = g_open(path, O_RDWR);
	if (fd >= 0) {
		res = fchown(fd, uid, gid);
		g_close(fd, NULL);
	}

	return res;
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
	gboolean done = FALSE;

	char *org_pwd = NULL;
	char *org_grp = NULL;
	int	 org_pwd_size = NAME_MAX;
	int	 org_grp_size = NAME_MAX;
	int	 new_uid, new_gid;

	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	struct group  get_grp;
	struct group  *res_grp;
	char	 *pwd_buf = NULL;
	char	 *grp_buf = NULL;
	int		 pwd_buf_size = PATH_MAX*2;
	int		 grp_buf_size = PATH_MAX*2;
	int    res;

	org_pwd = c_alloc(org_pwd_size);
	org_grp = c_alloc(org_grp_size);

	if (org_pwd == NULL || org_grp == NULL)
		goto finish;

	done = sys_file_get_owner_group(path,
																	org_pwd, org_pwd_size,
																	org_grp, org_grp_size);
	if (done == FALSE)
		goto finish;

	done = FALSE;
	pwd_buf = c_alloc(pwd_buf_size);
	grp_buf = c_alloc(grp_buf_size);
	if (pwd_buf == NULL || grp_buf == NULL)
		goto finish;

	errno = 0;
	if (c_strlen(owner, NAME_MAX) <= 0) {
		owner = org_pwd;
		new_uid = -1;
	}
	else {
		res = getpwnam_r(owner, &get_pwd, pwd_buf, pwd_buf_size, &res_pwd);
		if (res < 0 || res_pwd == NULL) {
			goto finish;
		}
		new_uid = get_pwd.pw_uid;
	}

	if (c_strlen(group, NAME_MAX) <= 0) {
		group = org_grp;
		new_gid = -1;
	}
	else {
		res = getgrnam_r(group, &get_grp, grp_buf, grp_buf_size, &res_grp);
		if (res < 0 || res_grp == NULL)
			goto finish;
		new_gid = get_grp.gr_gid;
	}

	done = TRUE;
	if (c_strcmp(org_pwd, owner, NAME_MAX, -1) == 0) {
		if (c_strcmp(org_grp, group, NAME_MAX, -1) == 0) {
			// no change
			done = TRUE;
		}
		else { // different group
			if (s_chown(path, -1, new_gid) < 0)
				done = FALSE;
		}
	}
	else {
		if (c_strcmp(org_grp, group, NAME_MAX, -1) == 0) { // different owner
			if (s_chown(path, new_uid, -1) < 0)
				done = FALSE;
		}
		else { // different owner, group
			if (s_chown(path, new_uid, new_gid) < 0)
				done = FALSE;
		}
	}

finish:
	c_free(&org_pwd);
	c_free(&org_grp);

	c_free(&pwd_buf);
	c_free(&grp_buf);

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
	gboolean done = FALSE;
	struct	stat finfo;

	struct passwd  get_pwd;
	struct passwd  *res_pwd;
	struct group  get_grp;
	struct group  *res_grp;
	char	 *pwd_buf = NULL;
	char	 *grp_buf = NULL;
	int		 pwd_buf_size = PATH_MAX*2;
	int		 grp_buf_size = PATH_MAX*2;
	int    res;

	if (stat(path, &finfo) < 0)
		return FALSE;

	pwd_buf = c_alloc(pwd_buf_size);
	grp_buf = c_alloc(grp_buf_size);
	if (pwd_buf == NULL || grp_buf == NULL)
		goto finish;

	errno = 0;
	res = getpwuid_r(finfo.st_uid, &get_pwd, pwd_buf, pwd_buf_size, &res_pwd);
	if (res < 0 || res_pwd == NULL)
		goto finish;

	res = getgrgid_r(finfo.st_gid, &get_grp, grp_buf, grp_buf_size, &res_grp);
	if (res < 0 || res_grp == NULL)
		goto finish;

	if (oSize < c_strlen(get_pwd.pw_name, NAME_MAX)+1)
		goto finish;

	if (gSize < c_strlen(get_grp.gr_name, NAME_MAX)+1)
		goto finish;

	done = TRUE;
	c_strcpy(owner, get_pwd.pw_name, NAME_MAX);
	c_strcpy(group, get_grp.gr_name, NAME_MAX);

finish:
	c_free(&pwd_buf);
	c_free(&grp_buf);

	return done;
}


static int g_mode_mask[9] = {
	S_IRUSR, S_IWUSR, S_IXUSR,
	S_IRGRP, S_IWGRP, S_IXGRP,
	S_IROTH, S_IWOTH, S_IXOTH
};
static char* g_mode_char = "rwxrwxrwx";

static gboolean _file_set_mode(gchar *path, char *modeStr, int off, int len)
{
	if (modeStr == NULL || c_strlen(modeStr, off+len) < off+len)
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

	if (g_chmod(path, mode) < 0)
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
	if (c_strmatch(path, "/") == 0)
		return TRUE;

	// already existing
	if (sys_file_is_existing(path))
		return TRUE;

	ptr = c_strrchr(path, '/', PATH_MAX);
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
		if (g_chmod(path, mode) != 0) {
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
		tmp = c_alloc(size);
		if (tmp) {
			int n;
			c_strcpy(tmp, path, size);
			n = c_strlen(tmp, PATH_MAX);
			if (tmp[n-1] == '/')
				tmp[n-1] = 0;
			done = _do_make_directory(tmp, owner, group, mode);
			c_free(&tmp);
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
		if (g_access(path, F_OK) == 0) {
			exist = TRUE;
		}
	}

	return exist;
}

int		sys_file_open(char *path, char *mode)
{
	int	fd = -1;

	if (c_strlen(path, PATH_MAX) > 0) {
		FILE *fp;

		fp = g_fopen(path, mode);
		if (fp != NULL) {
			fd = fileno(fp);
		}
	}

	return fd;
}

int		sys_file_read(int fd, char *buf, int size)
{
	int	n = -1;

	if (fd >= 0) {
		n = read(fd, buf, size);
	}

	return n;
}

void	sys_file_close(int *pfd)
{
	if (pfd) {
		int fd = *pfd;
		if (fd >= 0)
			g_close(fd, NULL);
		*pfd = -1;
	}
}

