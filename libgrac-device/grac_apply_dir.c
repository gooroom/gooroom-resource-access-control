/*
 * grac_apply_dir.c
 *
 *  Created on: 2016. 1. 15.
 *  Modified on : 2016. 09. 05
 *      Author: user
 */

/**
  @file 	 	grac_apply_dir.c
  @brief		각 리소스별 접근제어를 시스템에 적용하는 함수 모음
  @details	UID 기반으로  리소스 접근 제어를 수행한다. \n
         		프린터, 네트워크, 화면캡쳐, USB, 홈디렉토리에 대한 접근 제어를 수행한다. \n
  				헤더파일 :  	grac_apply_dir.h	\n
  				라이브러리:	libgrac.so
 */

#include "grac_apply_dir.h"

#include "sys_user.h"
#include "sys_iptables.h"
#include "sys_cups.h"
#include "sys_acl.h"
#include "sys_file.h"
#include "sys_user_list.h"

#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <grp.h>


/*
 *  File System
 *
 *       - only process Home Directory at now
 */


/**
  @brief  파일 접근 제어를 위한 초기화
  @details
       파일 접근 제어는 개별적으로 적용이 가능하다 \n
       사용예 \n
  @code {.c}
       		gboolean res;
       		uid_t 	home_uid;	// home dircetory owner
       		uid_t		set_uid;		// home directory에 접근하고자하는 user id
			res = grac_apply_dir_init();
			res = grac_apply_dir_home_init();
			res = grac_apply_dir_home_allow_by_uid(home_uid, set_uid, TRUE); 	// 적용하고자하는 사용자 id별로 반복 호출
			grac_apply_dir_clear();
  @endcode
  @return gboolean	성공여부
  @warning	홈 이외의 대상 파일, 디렉토리 지정이 가능한 기능 추가가 필요하다.
*/
gboolean grac_apply_dir_init()
{
	return TRUE;
}

/**
  @brief  파일 접근 제어를 위한 마무리
  @details
       내부 버퍼 해제등 파일 접근 제어를 마무리 한다. 적용된 상태는 유지된다.
*/
void     grac_apply_dir_clear()
{

}

// 2016.4.
/**
  @brief  홈디렉토리 접근 제어를 위한 초기화
  @details
       홈디렉토리 접근 제어는 개별적으로 적용이 가능하다 \n
       사용 예는 grac_apply_dir_init() 를 참조한다.
  @return gboolean	성공여부
*/
gboolean grac_apply_dir_home_init()
{
	struct sys_user_list* user_list;
	int		idx, count;
	char	*user_name;
	char	home_dir[128];

	user_list = sys_user_list_alloc(SYS_USER_KIND_GENERAL);
	count = sys_user_list_count(user_list);
	for (idx=0; idx<count; idx++) {
		gboolean res = FALSE;
		user_name = sys_user_list_get_name(user_list, idx);
		if (user_name != NULL)
			res = sys_file_get_default_home_dir(user_name, home_dir, sizeof(home_dir));
		if (res) {
			// printf("%s\n", home_dir);
			if (access(home_dir, F_OK) == 0) {
				sys_acl_clear(home_dir);
				sys_file_set_mode(home_dir, "rwxr-xr-x");
			}
		}
	}
	sys_user_list_free(&user_list);

	return TRUE;
}

/**
  @brief  지정된 사용자의 홈 디렉토리에 대한 다른 사용자의 접근 허용 여부를 설정한다.
  @details
       홈 디렉토리 접근 제어는 개별 적용이 가능하다.
  @param	[in]	home_uid	홈 디렉토리 소유자 리눅스 사용자ID
  @param	[in]	allow_uid	접근 허용 여부를 설정할 리눅스 사용자ID
  @param	[in]	allow		허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_dir_home_allow_by_uid (uid_t home_uid, uid_t allow_uid, gboolean allow)
{
	gboolean res = TRUE;
	gchar	homeUserName[128];
	gchar	allowUserName[128];
	gchar	*ptrAllowUser = NULL;

	res = sys_user_get_name_from_uid(home_uid, homeUserName, sizeof(homeUserName));
	if (allow_uid > 0) {
		res &= sys_user_get_name_from_uid(allow_uid, allowUserName, sizeof(allowUserName));
		if (res == TRUE)
			ptrAllowUser = allowUserName;
	}

	if (res == TRUE)
		res = grac_apply_dir_home_allow_by_user (homeUserName, ptrAllowUser, allow);

	return res;
}

/**
  @brief  지정된 사용자의 홈 디렉토리에 대한 다른 사용자의 접근 허용 여부를 설정한다.
  @details
       홈 디렉토리 접근 제어는 개별 적용이 가능하다.
  @param	[in]	home_user	홈 디렉토리 소유자 리눅스 사용자명
  @param	[in]	allow_user	접근 허용 여부를 설정할 리눅스 사용자
  @param	[in]	allow			허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_dir_home_allow_by_user(gchar *home_user, gchar* allow_user, gboolean allow)
{
	gchar	home_dir[128];
	gchar	home_owner[128];
	gchar	home_group[128];
	gboolean res;

	if (sys_file_get_default_home_dir(home_user, home_dir, sizeof(home_dir)) == FALSE)
			return FALSE;

	res = sys_file_get_owner_group(home_dir, home_owner, sizeof(home_owner),
			                                    home_group, sizeof(home_group) );
	if (res == FALSE)
		return FALSE;

	if (allow_user == NULL || strcmp(home_user, allow_user) == 0) {	// 설정하고자하는 사용자가 홈 디렉토리 소유자이다.
		// 파일 퍼미션 조정
		res &= sys_file_set_mode_other(home_dir, "---");
		if (allow == TRUE) {
			res &= sys_file_set_mode_user(home_dir, "rwx");
			//res &= sys_file_set_mode_group(home_dir, "r-x");
		}
		else {
			// 2016.3.11 add read permission to home even if deny-mode
			res &= sys_file_set_mode_user(home_dir, "r-x");
			//res &= sys_file_set_mode_user(home_dir, "---");
			//res &= sys_file_set_mode_group(home_dir, "---");
		}
	}
	else {  // 다른 사용자의 접근 허용 여부 설정
		// acl 설정
		res &= sys_file_set_mode_other(home_dir, "---");
		// 2016.3.11 add read permission to home even if deny-mode
		res &= sys_acl_set_user(home_dir, allow_user, "r-x");
		//if (allow == TRUE)
		//	res &= sys_acl_set_user(home_dir, allow_user, "r-x");
		//else
		//	res &= sys_acl_set_user(home_dir, allow_user, "---");
	}

	return res;
}

/**
  @brief  홈 디렉토리에 대한 그룹의 접근 허용 여부를 설정한다.
  @details
       홈 디렉토리 접근 제어는 개별 적용이 가능하다.	\n
       대상 홈 디렉토리는 시스템의 모든 일반 사용자의 홈디렉토리이다.
  @param	[in]	gid		접근 허용 여부를 설정할 리눅스 그룹ID
  @param	[in]	allow	허용 여부
  @return gboolean	성공여부
  @warning 그룹별 설정 필요성 검토 및 적용 방식 개선 필요
*/
gboolean grac_apply_dir_home_allow_by_gid (gid_t gid, gboolean allow)
{
	gboolean res = TRUE;
	gchar	grpName[128];

	res  = sys_user_get_name_from_gid(gid, grpName, sizeof(grpName));
	if (res == TRUE)
		res = grac_apply_dir_home_allow_by_group(grpName, allow);
	return res;
}

// step 1:  member of group
//          if same as home's group, change a group permission,
//          else use ACL to change a group permission
// step 2:  each home dir's group
//          if same as home's group, change a group permission,
//          else no operation
/**
  @brief  홈 디렉토리에 대한 그룹의 접근 허용 여부를 설정한다.
  @details
       홈 디렉토리 접근 제어는 개별 적용이 가능하다.	\n
       대상 홈 디렉토리는 시스템의 모든 일반 사용자의 홈디렉토리이다.
  @param	[in]	group_name		접근 허용 여부를 설정할 리눅스 그룹명
  @param	[in]	allow				허용 여부
  @return gboolean	성공여부
  @warning 그룹별 설정 필요성 검토 및 적용 방식 개선 필요
*/
gboolean grac_apply_dir_home_allow_by_group(gchar *group_name, gboolean allow)
{
	struct group  *grp;
	int		i;
	char	*user_name;
	int		main_uid;
	char	home_dir[128];
	gchar	home_user[128];
	gchar	home_group[128];
	gboolean res = TRUE;

	// user of main group
	main_uid = sys_user_get_main_uid (group_name);
	if (main_uid != -1)
		grac_apply_dir_home_allow_by_uid (main_uid, -1, allow);

	// user of supplementary group
	grp = getgrnam(group_name);
	if (grp == NULL) {
		return FALSE;
	}
	for (i = 0; grp->gr_mem[i] != NULL; i++) {
		gboolean get;
		user_name = grp->gr_mem[i];
		get = sys_file_get_default_home_dir(user_name, home_dir, sizeof(home_dir));
		if (get) {
			get = sys_file_get_owner_group(home_dir, home_user, sizeof(home_user),
					                                     home_group, sizeof(home_group) );
		}
		if (get) {
			res &= sys_file_set_mode_other(home_dir, "---");
			if (!strcmp(group_name, home_group)) {
				if (allow == TRUE)
					res &= sys_file_set_mode_group(home_dir, "r-x");
				else
					res &= sys_file_set_mode_group(home_dir, "---");
			}
			else {	// another group
				if (allow == TRUE)
					res &= sys_acl_set_group(home_dir, group_name, "r-x");
				else
					res &= sys_acl_set_group(home_dir, group_name, "---");
			}
		}
	 }

	// check the group of each home directory
	struct sys_user_list* user_list;
	int		idx, count;

	user_list = sys_user_list_alloc(SYS_USER_KIND_GENERAL);
	count = sys_user_list_count(user_list);
	for (idx=0; idx<count; idx++) {
		gboolean get = FALSE;
		user_name = sys_user_list_get_name(user_list, idx);
		if (user_name != NULL)
			get = sys_file_get_default_home_dir(user_name, home_dir, sizeof(home_dir));
		if (get) {
			get = sys_file_get_owner_group(home_dir, home_user, sizeof(home_user),
					                                     home_group, sizeof(home_group) );
		}
		if (get) {
			if (!strcmp(group_name, home_group)) {
				res &= sys_file_set_mode_other(home_dir, "---");
				if (allow == TRUE)
					res &= sys_file_set_mode_group(home_dir, "r-x");
				else
					res &= sys_file_set_mode_group(home_dir, "---");
			}
		}
	}
	sys_user_list_free(&user_list);

	return res;
}


#ifdef _method2
gboolean grac_apply_dir_home_allow_by_gid (uid_t home_uid, gid_t allow_gid, gboolean allow)
{
	gboolean res = TRUE;
	gchar	userName[128];
	gchar	grpName[128];
	gchar	*ptrAllowGroup = NULL;

	res  = sys_user_get_name_from_uid(home_uid, userName, sizeof(userName));
	if (allow_gid > 0) {
		res &= sys_user_get_name_from_gid(allow_gid, grpName, sizeof(grpName));
		if (res == TRUE)
			ptrAllowGroup = grpName;
	}

	if (res == TRUE)
		res = grac_apply_dir_home_allow_by_group(userName, ptrAllowGroup, allow);
	return res;
}

gboolean grac_apply_dir_home_allow_by_group(gchar *home_user, gchar *allow_group, gboolean allow)
{
	gchar	homeDir[128];
	gchar	homeOwner[128];
	gchar	homeGroup[128];
	gboolean res;

	if (sys_file_get_default_home_dir(home_user, homeDir, sizeof(homeDir)) == FALSE)
			return FALSE;

	res = sys_file_get_owner_group(homeDir, homeOwner, sizeof(homeOwner),
			                                    homeGroup, sizeof(homeGroup) );
	if (res == FALSE)
		return FALSE;

	res = TRUE;
	if (allow_group == NULL || strcmp(homeGroup, allow_group) == 0) {	// file's default group
		if (allow == TRUE) {
			res &= sys_file_set_mode_group(homeDir, "r-x");
		}
		else {
			res &= sys_file_set_mode_group(homeDir, "---");
		}
	}
	else {	// another group
		if (allow == TRUE)
			res &= sys_acl_set_group(homeDir, allow_group, "r-x");
		else
			res &= sys_acl_set_group(homeDir, allow_group, "---");
	}

	return res;
}
#endif

