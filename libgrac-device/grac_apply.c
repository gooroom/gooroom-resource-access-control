/*
 * grac_access_apply.c
 *
 */

/**
  @file 	 	grac_access_apply.c
  @brief		권한에 따른 리소스 접근제어를 시스템에 적용
  @details	UID 기반으로  리소스 접근 제어를 수행한다. \n
			    권한 적용 전체의 제어를 이 곳에서 수행하며 각각의 리소스별 접근 제어 설정은 grac_apply를 참조한다. \n
  				헤더파일 :  	grac_access_apply.h	\n
  				라이브러리:	libgrac.so
 */


#include "grac_config.h"
#include "grac_access.h"
#include "grac_apply.h"
#include "sys_user.h"
#include "sys_user_list.h"
#include "grm_log.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

// printer의 접근 제어는  cups를 이용하는데
// 사용자 전체를 한 번에 설정해야 하므로 사용자 목록을 등록 후 적용을 수행한다.
static gboolean _acc_apply_printer_rules (GracAccess *access, gchar *user)
{
	gboolean done = FALSE;
	int	uid;

	// 사용자 목록 초기화, 기타 초기 처리
	done = grac_apply_printer_access_init();
	if (done == FALSE)
		return FALSE;

	// 사용자  등록
	uid = sys_user_get_uid_from_name(user);
	if (grac_access_get_allow(access, GRAC_ATTR_PRINTER))
		grac_apply_printer_access_add_uid (uid, TRUE);
	else
		grac_apply_printer_access_add_uid (uid, FALSE);

	// root 사용자는??

	// Apply only allowed users & groups
	done = grac_apply_printer_access_apply (TRUE);	// 일괄적용, 프린터 사용 허용 사용자를 시스템에 적용

	grac_apply_printer_access_end();

	return done;
}

// network 제어는  iptables를 이용하고 사용자를 개별적으로 적용요청이 가능하다.
/*
static gboolean _acc_apply_network_rules (GracAccess *access, gchar *user)
{
	gboolean done = FALSE;

	done = grac_apply_network_init();
	if (done == FALSE)
		return FALSE;

	if (grac_access_get_allow(access, GRAC_ATTR_NETWORK))
		done = grac_apply_network_allow_by_user(user, TRUE);
	else
		done = grac_apply_network_allow_by_user(user, FALSE);

	grac_apply_network_end();

	return done;
}
*/

// 화면 캡쳐 프로그램을 외부에서 정의하게 변경 필요 !!!!!
static gboolean _acc_apply_capture_rules (GracAccess *access, gchar *user)
{
	gboolean done = FALSE;
	int	uid;

	done = grac_apply_capture_init("/usr/bin/xfce4-screenshooter");		// Todo: from config
	if (done == FALSE)
		return FALSE;

	uid = sys_user_get_uid_from_name(user);
	if (uid > 0) {  // 개별 적용
		if (grac_access_get_allow(access, GRAC_ATTR_SCREEN_CAPTURE))
			done = grac_apply_capture_allow_by_uid(uid, TRUE);
		else
			done = grac_apply_capture_allow_by_uid(uid, FALSE);
	}

	grac_apply_capture_end();

	return done;
}


// 일단은 home directory만을 대상으로 한다.
// 2016.4  change to process groups first
/*
static gboolean _acc_apply_harddisk_rules (GracAccess *access, gchar *user)
{
	gboolean done = FALSE;

	done = grac_apply_dir_init();
	if (done == FALSE)
		return FALSE;

	// 2016.4
	grac_apply_dir_home_init();

	// user by user
	int uid = sys_user_get_uid_from_name(user);
	if (uid > 0 && sys_user_check_home_dir_by_name(user)) {
		if (grac_access_get_allow(access, GRAC_ATTR_HOME_DIR))
			done &= grac_apply_dir_home_allow_by_user(user, NULL, TRUE);
		else
			done &= grac_apply_dir_home_allow_by_user(user, NULL, FALSE);
	}

	grac_apply_dir_end();

	return done;
}
*/

// USB storage의 사용권 제어
static gboolean _acc_apply_usb_rules (GracAccess *access, char *username)
{
	gboolean done = TRUE;

	done = grac_apply_usb_init();
	if (done == FALSE)
		return FALSE;

	if (grac_access_get_allow(access, GRAC_ATTR_USB))
			done &= grac_apply_usb_storage_allow_by_user(username, NULL, TRUE);
	else
			done &= grac_apply_usb_storage_allow_by_user(username, NULL, FALSE);


	grac_apply_usb_end();

	return done;
}

/**
  @brief  사용자 목록에 의한 리소스 접근 권한을 시스템에 반영한다.
  @details
       프린터, 네트워크, 화면캡쳐, USB, 홈디렉토리에 대한 접근 제어를 수행한다. \n
       이 함수에서는 각 사용자별 용도는 모르는 상태이며 단지 각 사용자의 uid를 이용하여 접근제어를 수행한다.
  @param [in]	list	사용자및 권한 목록 (시스템 상의 일반 사용자와 권한제어용 사용자)
  @return gboolean	성공여부
*/
gboolean grac_apply_access_by_user(GracAccess *access, char *username)
{
	gboolean done = TRUE;
	gboolean res;
	char	*func = "grac_access_apply_by_user()";

	res = _acc_apply_printer_rules (access, username);
	done &= res;
	if (res == FALSE)
		grm_log_error("%s : can't apply for printer", func);

//	res = _acc_apply_network_rules (access, username);
//	done &= res;
//	if (res == FALSE)
//		grm_log_error("%s : can't apply for network", func);

	res = _acc_apply_capture_rules (access, username);
	done &= res;
	if (res == FALSE)
		grm_log_error("%s : can't apply for capture", func);

//	res = _acc_apply_harddisk_rules (access, username);
//	done &= res;
//	if (res == FALSE)
//		grm_log_error("%s : can't apply for Home", func);

	res = _acc_apply_usb_rules (access, username);
	done &= res;
	if (res == FALSE)
		grm_log_error("%s : can't apply for USB", func);

	return res;
}

