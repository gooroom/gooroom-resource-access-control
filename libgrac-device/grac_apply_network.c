/*
 * grac_apply_network.c
 *
 *  Created on: 2016. 1. 15.
 *  Modified on : 2016. 09. 05
 *      Author: user
 */

/**
  @file 	 	grac_apply_network.c
  @brief		각 리소스별 접근제어를 시스템에 적용하는 함수 모음
  @details	UID 기반으로  리소스 접근 제어를 수행한다. \n
         		프린터, 네트워크, 화면캡쳐, USB, 홈디렉토리에 대한 접근 제어를 수행한다. \n
  				헤더파일 :  	grac_apply_network.h	\n
  				라이브러리:	libgrac-device.so
 */

#include "grac_apply_network.h"

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


/**
  @brief  네트워크 접근 제어를 위한 초기화
  @details
       네트워크는 사용자 개별적으로 적용 가능하다. \n
       사용예 \n
  @code {.c}
       		gboolean res;
       		uid_t 	uid;
			res = grac_apply_network_init();
			res = grac_apply_network_allow_by_uid(uid, TRUE);		// 설정하고자하는 사용자 id별로 반복 호출
			grac_apply_network_clear();
  @endcode
  @return gboolean	성공여부
*/
gboolean grac_apply_network_init()
{
	return sys_iptbl_init();
}

/**
  @brief  네트워크 접근 제어를 위한 마무리
  @details
       네트워크는 접근제어를 마무리 한다. 적용된 상태는 유지된다.
*/
void     grac_apply_network_end()
{

}

/**
  @brief  지정한 사용자에 대한 네트워크 접근 제어를 시스템에 적용
  @details
       네트워크는 사용자 개별적으로 적용 가능하다.
  @param	[in]	uid		사용자 ID (리눅스 사용자 ID)
  @param	[in]	allow	접근 허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_network_allow_by_uid(uid_t uid, gboolean allow)
{
	int action;

	if (allow == TRUE)
		action = IPTBL_ACTION_ACCEPT;
	else
		action = IPTBL_ACTION_DROP;

	return sys_iptbl_set_action_by_uid (uid, IPTBL_FILTER_OUTPUT, action);
}

/**
  @brief  지정한 그룹에 대한 네트워크 접근 제어를 시스템에 적용
  @details
       네트워크는 그룹 개별적으로 적용 가능하다.
  @param	[in]	gid		그룹 ID (리눅스 그룹 ID)
  @param	[in]	allow	접근 허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_network_allow_by_gid(uid_t gid, gboolean allow)
{
	int action;

	if (allow == TRUE)
		action = IPTBL_ACTION_ACCEPT;
	else
		action = IPTBL_ACTION_DROP;

	return sys_iptbl_set_action_by_gid (gid, IPTBL_FILTER_OUTPUT, action);
}

/**
  @brief  지정한 사용자에 대한 네트워크 접근 제어를 시스템에 적용
  @details
       네트워크는 사용자 개별적으로 적용 가능하다.
  @param	[in]	user		사용자명
  @param	[in]	allow	접근 허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_network_allow_by_user (char *user,  gboolean allow)
{
	int action;

	if (allow == TRUE)
		action = IPTBL_ACTION_ACCEPT;
	else
		action = IPTBL_ACTION_DROP;

	return sys_iptbl_set_action_by_user (user, IPTBL_FILTER_OUTPUT, action);
}

/**
  @brief  지정한 그룹에 대한 네트워크 접근 제어를 시스템에 적용
  @details
       네트워크는 그룹 개별적으로 적용 가능하다.
  @param	[in]	group	그룹명 (리눅스 그룹명)
  @param	[in]	allow	접근 허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_network_allow_by_group(char *group, gboolean allow)
{
	int action;

	if (allow == TRUE)
		action = IPTBL_ACTION_ACCEPT;
	else
		action = IPTBL_ACTION_DROP;

	return sys_iptbl_set_action_by_group (group, IPTBL_FILTER_OUTPUT, action);
}
