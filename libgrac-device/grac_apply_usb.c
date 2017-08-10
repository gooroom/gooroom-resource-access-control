/*
 * grac_apply_usb.c
 *
 *  Created on: 2016. 1. 15.
 *  Modified on : 2016. 09. 05
 *      Author: user
 */

/**
  @file 	 	grac_apply_usb.c
  @brief		각 리소스별 접근제어를 시스템에 적용하는 함수 모음
  @details	UID 기반으로  리소스 접근 제어를 수행한다. \n
         		프린터, 네트워크, 화면캡쳐, USB, 홈디렉토리에 대한 접근 제어를 수행한다. \n
  				헤더파일 :  	grac_apply_usb.h	\n
  				라이브러리:	libgrac-device.so
 */

#include "grac_apply_usb.h"

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
 *****************************************************
 *   USB strorage
*****************************************************
 */

#define GOOROOM_USB_GROUP_NAME    "usb"

/*
static gboolean _run_cmd (char *cmd)   // 임시
{
	gboolean res;
	FILE *fp;

	fp = popen(cmd, "r");
	if (fp != NULL) {
		//char buf[1024];
		//while (1) {
		// 	if (fgets(buf, sizeof(buf), fp) == NULL)
		//		break;
		//	printf("----- %s", buf);
		//}
		pclose(fp);
		res = TRUE;
	}
	else {
		res = FALSE;
	}

	return res;
}
*/

/*
static gboolean _make_usb_storage_home(gchar* homeDir)
{
	if (access(homeDir, F_OK) != 0) {
		if (errno == ENOENT) {
			if (mkdir(homeDir, 0755) != 0)
				return FALSE;
		}
		else {
			return FALSE;
		}
	}

	gboolean res;
	gchar	homeOwner[128];
	gchar	homeGroup[128];
	gchar* defaultUser = "root";
	res = sys_file_get_owner_group(homeDir, homeOwner, sizeof(homeOwner),
			                                    homeGroup, sizeof(homeGroup) );
	if (res) {
		if (strcmp(homeOwner, defaultUser) != 0 || strcmp(homeGroup, defaultUser) != 0) {
			res = sys_file_change_owner_group(homeDir, defaultUser, defaultUser);
		}
	}

	//if (res)
	//	res = sys_acl_set_group(homeDir, group, "rwx");

	return res;
}
*/

static gboolean _init_usb_storage_home(gchar* user_name)
{
	char	usb_group[256];
	char	usb_home[256];
	gboolean get = TRUE;
	gboolean res;

//	if (grac_sys_user_is_valid_user (user_name))
//		return TRUE;

	if (sys_user_get_uid_from_name(user_name) <= 0)
		return TRUE;


	get = sys_user_get_main_group_from_name(user_name, usb_group, sizeof(usb_group));
	get &= sys_file_get_default_usb_dir(user_name, usb_home, sizeof(usb_home));

	if (access(usb_home, F_OK) != 0) {
		if (errno == ENOENT) {
			if (mkdir(usb_home, 0750) != 0)  // 755
				return FALSE;
		}
		else {
			return FALSE;
		}
	}

	gchar	homeOwner[128];
	gchar	homeGroup[128];
	gchar* defaultUser = "root";
	res = sys_file_get_owner_group(usb_home, homeOwner, sizeof(homeOwner),
			                                    homeGroup, sizeof(homeGroup) );
	if (res) {
		if (strcmp(homeOwner, defaultUser) != 0 || strcmp(homeGroup, defaultUser) != 0) {
			res = sys_file_change_owner_group(usb_home, defaultUser, defaultUser);
		}
	}

	if (res) {
		res = sys_acl_clear(usb_home);
		res &= sys_acl_set_user(usb_home, user_name, "rwx");
		res &= sys_acl_set_group(usb_home, usb_group, "rwx");
		res &= sys_acl_set_other(usb_home, "---");
	}

	return res;
}

/**
  @brief  USB에 대한 접근 제어를 위한 초기화
  @details
       USB 접근 제어는 개별적으로 적용이 가능하다 \n
       구름OS는 USB 삽입시 로그인 사용자 소유의 디렉토리로 생성한다.
       사용예 \n
  @code {.c}
       		gboolean res;
       		uid_t 	usb_uid;		// USB 소유자 즉 로그인 사용자 ID
       		uid_t 	set_uid;		// 접근허용  여부를 설정할 사용자 ID
			res = grac_apply_usb_init();
			res = grac_apply_usb_storage_allow_by_uid  (usb_uid, set_uid, TRUE); 	// 설정하고자하는 사용자 id별로 반복 호출
			grac_apply_usb_clear();
  @endcode
  @return gboolean	성공여부

*  2016.1.31  usb group handling
*/
gboolean grac_apply_usb_init()
{
	// chcek if group 'usb'
	/*
	if (sys_user_get_gid_from_name(GOOROOM_USB_GROUP_NAME) < 0) { // not found
		char cmd[128];
		sprintf(cmd, "groupadd %s", GOOROOM_USB_GROUP_NAME);
		res &= _run_cmd(cmd);
	}
    */

	struct sys_user_list* user_list;
	int		idx, count;
	char	*user_name;

	user_list = sys_user_list_alloc(SYS_USER_KIND_GENERAL);
	count = sys_user_list_count(user_list);
	for (idx=0; idx<count; idx++) {
		user_name = sys_user_list_get_name(user_list, idx);
		if (user_name == NULL)
				continue;
		_init_usb_storage_home(user_name);
	}
	sys_user_list_free(&user_list);

	return TRUE;
}

/**
  @brief  USB 접근 제어를 위한 마무리
  @details
       내부 버퍼 해제등 파일 접근 제어를 마무리 한다. 적용된 상태는 유지된다.
*/
void     grac_apply_usb_end()
{
	// no action at now
}

/**
  @brief  USB storage에 대하여 지정된 사용자의 접근 허용 여부를 설정한다.
  @details
       USB storage 접근 제어는 개별 적용이 가능하다.	\n
       USB storage는 로그인 사용자의 소유로 생성된다.
  @param	[in]	usb_uid	USB storage 소유자의 사용자 ID
  @param	[in]	allow_uid	접근 허용 여부를 설정할  사용자 ID
  @param	[in]	allow		허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_usb_storage_allow_by_uid  (uid_t  usb_uid, uid_t allow_uid, gboolean allow)
{
	gboolean res = TRUE;
	gchar	usbUser[128];
	gchar	allowUser[128];

	res  = sys_user_get_name_from_uid(usb_uid,  usbUser, sizeof(usbUser));
	res &= sys_user_get_name_from_uid(allow_uid, allowUser, sizeof(allowUser));
	if (res == TRUE)
		res = grac_apply_usb_storage_allow_by_user(usbUser, allowUser, allow);

	return res;
}

/**
  @brief  USB storage에 대하여 지정된 사용자의 접근 허용 여부를 설정한다.
  @details
       USB storage 접근 제어는 개별 적용이 가능하다.	\n
       USB storage는 로그인 사용자의 소유로 생성된다.
  @param	[in]	usb_user		USB storage 소유자의 사용자명
  @param	[in]	allow_user	접근 허용 여부를 설정할  사용자 명
  @param	[in]	allow			허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_usb_storage_allow_by_user (gchar* usb_user, gchar* allow_user, gboolean allow)
{
	gchar	usb_home[128];
	gboolean res = TRUE;

//	if (grac_sys_user_is_valid_user (usb_user))
//		return TRUE;

	if (sys_user_get_uid_from_name(usb_user) <= 0)
		return TRUE;

	if (allow_user == NULL)
		allow_user = usb_user;

	//printf("%s-->%s %d\n", usb_user, allow_user, (int)allow);

	res = sys_file_get_default_usb_dir(usb_user, usb_home, sizeof(usb_home));
	if (res == FALSE)
		return FALSE;

	gchar	homeOwner[128];
	gchar	homeGroup[128];
	res = sys_file_get_owner_group(usb_home, homeOwner, sizeof(homeOwner),
			                                     homeGroup, sizeof(homeGroup) );

	if (strcmp(allow_user, homeOwner) == 0) {
		if (allow == TRUE)
			res = sys_file_set_mode_user(usb_home, "rwx");
		else
			res = sys_file_set_mode_user(usb_home, "---");
	}
	else {
		if (allow == TRUE)
			res = sys_acl_set_user(usb_home, allow_user, "rwx");
		else
			res = sys_acl_set_user(usb_home, allow_user, "---");
	}

	return res;
}

/**
  @brief  USB storage에 대하여 지정된 그룹의 접근 허용 여부를 설정한다.
  @details
       USB storage 접근 제어는 개별 적용이 가능하다.	\n
       USB storage는 로그인 사용자의 소유로 생성된다.
  @param	[in]	usb_uid		USB storage 소유자의 사용자ID
  @param	[in]	allow_gid		접근 허용 여부를 설정할  그룹ID
  @param	[in]	allow			허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_usb_storage_allow_by_gid  (uid_t usb_uid, gid_t allow_gid, gboolean allow)
{
	gboolean res = TRUE;
	gchar	userName[128];
	gchar	grpName[128];

	res  = sys_user_get_name_from_uid(usb_uid, userName, sizeof(userName));
	res &= sys_user_get_name_from_gid(allow_gid, grpName, sizeof(grpName));
	if (res == TRUE)
		res = grac_apply_usb_storage_allow_by_group(userName, grpName, allow);
	return res;

}

/**
  @brief  USB storage에 대하여 지정된 그룹의 접근 허용 여부를 설정한다.
  @details
       USB storage 접근 제어는 개별 적용이 가능하다.	\n
       USB storage는 로그인 사용자의 소유로 생성된다.
  @param	[in]	usb_user		USB storage 소유자의 사용자명
  @param	[in]	allow_group	접근 허용 여부를 설정할  그룹명
  @param	[in]	allow			허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_usb_storage_allow_by_group(gchar* usb_user, gchar* allow_group, gboolean allow)
{
	gchar	usb_home[128];
	gboolean res = TRUE;

	res = sys_file_get_default_usb_dir(usb_user, usb_home, sizeof(usb_home));
//if (res)
//	res = _make_usb_storage_home(usb_home);
	if (res == FALSE)
		return FALSE;

	gchar	homeOwner[128];
	gchar	homeGroup[128];
	res = sys_file_get_owner_group(usb_home, homeOwner, sizeof(homeOwner),
			                                     homeGroup, sizeof(homeGroup) );

	if (allow_group == NULL)
		allow_group = usb_user;

	if (strcmp(allow_group, homeGroup) == 0) {
		if (allow == TRUE)
			res = sys_file_set_mode_group(usb_home, "rwx");
		else
			res = sys_file_set_mode_group(usb_home, "---");
	}
	else {
		if (allow == TRUE)
			res = sys_acl_set_group(usb_home, allow_group, "r-x");
		else
			res = sys_acl_set_group(usb_home, allow_group, "---");
	}
	return res;
}

