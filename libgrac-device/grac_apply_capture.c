/*
 * grac_apply_capture.c
 *
 *  Created on: 2016. 1. 15.
 *  Modified on : 2016. 09. 05
 *      Author: user
 */

/**
  @file 	 	grac_apply_capture.c
  @brief		각 리소스별 접근제어를 시스템에 적용하는 함수 모음
  @details	UID 기반으로  리소스 접근 제어를 수행한다. \n
         		프린터, 네트워크, 화면캡쳐, USB, 홈디렉토리에 대한 접근 제어를 수행한다. \n
  				헤더파일 :  	grac_apply.h	\n
  				라이브러리:	libgrac.so
 */

#include "grac_apply_capture.h"

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
 *  Capture
 *       - one time operation for all user or groups
 */


static char g_capture_app[1024] = { 0 };

/**
  @brief  화면 캡쳐에 대한 접근 제어를 위한 초기화
  @details
       화면 캡쳐는 개별적으로 적용이 가능하다 \n
       초기화 요청시 화면캡쳐에 사용되는 application의 full path를 전달한다.
       사용예 \n
  @code {.c}
       		gboolean res;
       		uid_t 	uid;
			res = grac_apply_capture_init("/usr/bin/capture");
			res = grac_apply_capture_apply_by_uid  (uid, TRUE); 	// 설정하고자하는 사용자 id별로 반복 호출
			grac_apply_capture_clear();
  @endcode
  @param [in]	path	화면캡쳐 프로그렘 경로명
  @return gboolean	성공여부
*/
gboolean grac_apply_capture_init(gchar *path)
{
	strncpy(g_capture_app, path, sizeof(g_capture_app) );

	// 2016.4.
	sys_acl_clear(g_capture_app);

	return TRUE;
}

/**
  @brief  화면 캡쳐 제어를 위한 마무리
  @details
       내부 버퍼 해제등 화면 캡쳐제어를 마무리 한다. 적용된 상태는 유지된다.
*/
void grac_apply_capture_end()
{

}

/**
  @brief  지정된 사용자의 화면캡쳐 허용 여부를 설정한다.
  @details
       화면캡쳐는 개별 적용이 가능하다.
  @param	[in]	uid		사용자 ID (리눅스 사용자 ID)
  @param	[in]	allow	허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_capture_allow_by_uid(uid_t uid, gboolean allow)
{
	if (g_capture_app[0] == 0)
		return FALSE;

	gboolean res;
	if (allow == TRUE)
		res = sys_acl_set_uid (g_capture_app, uid, "r-x");
	else
		res = sys_acl_set_uid (g_capture_app, uid, "---");\
	return res;
}

/**
  @brief  지정된 그룹의 화면캡쳐 허용 여부를 설정한다.
  @details
       화면캡쳐는 개별 적용이 가능하다.
  @param	[in]	gid		그룹 ID (리눅스 그룹 ID)
  @param	[in]	allow	허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_capture_allow_by_gid(uid_t gid, gboolean allow)
{
	if (g_capture_app[0] == 0)
		return FALSE;

	gboolean res;
	if (allow == TRUE)
		res = sys_acl_set_gid (g_capture_app, gid, "r-x");
	else
		res = sys_acl_set_gid (g_capture_app, gid, "---");\
	return res;
}

/**
  @brief  지정된 사용자의 화면캡쳐 허용 여부를 설정한다.
  @details
       화면캡쳐는 개별 적용이 가능하다.
  @param	[in]	user		사용자명  (리눅스 사용자 명)
  @param	[in]	allow	허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_capture_allow_by_user (char *user,  gboolean allow)
{
	if (g_capture_app[0] == 0)
		return FALSE;

	gboolean res;
	if (allow == TRUE)
		res = sys_acl_set_user(g_capture_app, user, "r-x");
	else
		res = sys_acl_set_user(g_capture_app, user, "---");\
	return res;
}

/**
  @brief  지정된 그룹의 화면캡쳐 허용 여부를 설정한다.
  @details
       화면캡쳐는 개별 적용이 가능하다.
  @param	[in]	group	그룹명 (리눅스 그룹명)
  @param	[in]	allow	허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_capture_allow_by_group(char *group, gboolean allow)
{
	if (g_capture_app[0] == 0)
		return FALSE;

	gboolean res;
	if (allow == TRUE)
		res = sys_acl_set_group(g_capture_app, group, "r-x");
	else
		res = sys_acl_set_group(g_capture_app, group, "---");\
	return res;
}


