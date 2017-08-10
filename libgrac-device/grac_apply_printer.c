/*
 * grac_apply_printer.c
 *
 *  Created on: 2016. 1. 15.
 *  Modified on : 2016. 09. 05
 *      Author: user
 */

/**
  @file 	 	grac_apply_printer.c
  @brief		각 리소스별 접근제어를 시스템에 적용하는 함수 모음
  @details	UID 기반으로  리소스 접근 제어를 수행한다. \n
         		프린터, 네트워크, 화면캡쳐, USB, 홈디렉토리에 대한 접근 제어를 수행한다. \n
  				헤더파일 :  	grac_apply_printer.h	\n
  				라이브러리:	libgrac-device.so
 */

#include "grac_apply_printer.h"

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
 *  Printer
 *       - one time operation for all user or groups
 */

typedef struct _GrmPrinterAccData GrmPrinterAccData;
struct _GrmPrinterAccData {
	gboolean allow;
	gboolean isUserID;
	int		 id;
	char	*name;
};

typedef struct _GrmPrinterAccess {
	GPtrArray *list;
} GrmPrinterAccess;

static GrmPrinterAccess G_PrinterAccess = { 0 };


static GrmPrinterAccData* _alloc_data(int nameSize)
{
	GrmPrinterAccData* data;

	data = malloc(sizeof(GrmPrinterAccData));
	if (data != NULL) {
		memset(data, 0, sizeof(GrmPrinterAccData));
		data->name = malloc(nameSize+1);
		if (data->name == NULL) {
			free(data);
			data = NULL;
		}
		else {
			*data->name = 0;
		}
	}
	return data;
}

static void _free_data (GrmPrinterAccData* data)
{
	if (data != NULL) {
		if (data->name == NULL) {
			free(data->name);
		}
		free(data);
	}
}


static void _free_buf()
{
	if (G_PrinterAccess.list != NULL) {
		int	i;
		for (i=0; i<G_PrinterAccess.list->len; i++) {
			GrmPrinterAccData* data;
			data = (GrmPrinterAccData*)g_ptr_array_index(G_PrinterAccess.list, i);
			_free_data(data);
		}
		gpointer gptr = g_ptr_array_free(G_PrinterAccess.list, FALSE);
		if (gptr)
			g_free(gptr);

		G_PrinterAccess.list = NULL;
	}
}


/**
  @brief  프린터 접근 제어를 위한 초기화
  @details
       프린터는 개별적으로 적용이 불가능하고 사용자 전체를 한번에 적용해야 한다 \n
       따라서 적용 전에 사용자 혹은 그룹 목록을 등록하고 적용을 요청한다. 목록은 사용자 혹은 그룹에 관계없이 혼용해서 등록가능하다.
       사용예 \n
  @code {.c}
       		gboolean res;
       		uid_t 	uid;
			res = grac_apply_printer_access_init();
			res = grac_apply_printer_access_add_uid  (uid, TRUE); 	// 설정하고자하는 사용자 id별로 반복 호출
			res = grac_apply_printer_access_apply(TRUE);				// 사용자 혹은 그룹 등록이 완료되면 시스템으로의 반영을 요청한다.
			grac_apply_printer_access__clear();
  @endcode
  @return gboolean	성공여부
*/
gboolean grac_apply_printer_access_init()
{
	if (G_PrinterAccess.list == NULL) {
		G_PrinterAccess.list = g_ptr_array_new();
		if (G_PrinterAccess.list == NULL)
			return FALSE;
	}

	gboolean res;
	res = sys_cups_access_init();
	if (res == FALSE)
		_free_buf();

	return res;
}

/**
  @brief  프린터  접근 제어를 위한 마무리
  @details
       내부 버퍼 해제등 프린터 접근제어를 마무리 한다. 적용된 상태는 유지된다.
*/
void  grac_apply_printer_access_end()
{
	_free_buf();

	sys_cups_access_end();
}

/**
  @brief  등록된 사용자,그룹 전체에 대해 일괄적으로 프린터 접근 제어를 시스템에 적용
  @details
       프린터는 일괄 적용만 가능하다. \n
       등록시에는 접근허용 여부에 관계없이 모두 등록 가능하나 \n
       적용시 파라메터로 지정된 접근 여부와 동일한 사용자,그룹만을 대상으로 한다.
  @param	[in]	allow	접근 허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_printer_access_apply(gboolean allow)
{
	if (G_PrinterAccess.list == NULL)
		return FALSE;

	int	access;
	int	i;
	gboolean res = TRUE;

	if (allow == TRUE)
		access = CUPS_ACCESS_ALLOW;
	else
		access = CUPS_ACCESS_DENY;

	for (i=0; i<G_PrinterAccess.list->len; i++) {
		GrmPrinterAccData* data;
		data = g_ptr_array_index(G_PrinterAccess.list, i);
		if (data->allow == allow) {
			if (data->isUserID == TRUE) {
				res &= sys_cups_access_add_user (data->name, access);
			}
			else {
				res &= sys_cups_access_add_group(data->name, access);
			}
		}
	}
	if (res == TRUE)
		res = sys_cups_access_apply(access);

	return res;
}

static gboolean _printer_access_add_data(gboolean userB, int id, char* name, gboolean allow)
{
	GrmPrinterAccData* data;
	data = _alloc_data(strlen(name)+1);
	if (data != NULL) {
		data->isUserID = userB;
		data->id = id;
		strcpy(data->name, name);
		data->allow = allow;
		g_ptr_array_add(G_PrinterAccess.list, data);
		return TRUE;
	}

	return FALSE;
}

/**
  @brief  프린터 접근 제어를 할 사용자를 등록한다.
  @details
       프린터는 일괄 적용만 가능하므로 적용 요청 전에 사용자를 등록해 놓는다.
  @param	[in]	uid		사용자 ID (리눅스 사용자 ID)
  @param	[in]	allow	접근 허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_printer_access_add_uid  (uid_t uid, gboolean allow)
{
	gboolean res = FALSE;
	gchar	name[128];

	res = sys_user_get_name_from_uid(uid, name, sizeof(name));
	if (res == TRUE)
		res = _printer_access_add_data(TRUE, uid, name, allow);

	return res;
}

/**
  @brief  프린터 접근 제어를 할 그룹을 등록한다.
  @details
       프린터는 일괄 적용만 가능하므로 적용 요청 전에 그룹을 등록해 놓는다.
  @param	[in]	gid		그룹 ID (리눅스 그룹 ID)
  @param	[in]	allow	접근 허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_printer_access_add_gid  (uid_t gid, gboolean allow)
{
	gboolean res = FALSE;
	gchar	name[128];

	res = sys_user_get_name_from_gid(gid, name, sizeof(name));
	if (res == TRUE)
		res = _printer_access_add_data(FALSE, gid, name, allow);

	return res;
}

/**
  @brief  프린터 접근 제어를 할 사용자를 등록한다.
  @details
       프린터는 일괄 적용만 가능하므로 적용 요청 전에 사용자를 등록해 놓는다.
  @param	[in]	user		사용자명 (리눅스 사용자명)
  @param	[in]	allow	접근 허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_printer_access_add_user (char *user,  gboolean allow)
{
	gboolean res = FALSE;
	uid_t uid = sys_user_get_uid_from_name(user);
	if (uid > 0)
		res = _printer_access_add_data(TRUE, uid, user, allow);

	return res;
}

/**
  @brief  프린터 접근 제어를 할 그룹을 등록한다.
  @details
       프린터는 일괄 적용만 가능하므로 적용 요청 전에 그룹을 등록해 놓는다.
  @param	[in]	group	그룹명 (리눅스 그룹명)
  @param	[in]	allow	접근 허용 여부
  @return gboolean	성공여부
*/
gboolean grac_apply_printer_access_add_group(char *group, gboolean allow)
{
	gboolean res = FALSE;
	gid_t gid = sys_user_get_gid_from_name(group);
	if (gid > 0)
		res = _printer_access_add_data(FALSE, gid, group, allow);

	return res;
}

static gboolean _printer_access_del_data(gboolean userB, int id, char* name)
{
	GrmPrinterAccData* data;

	if (G_PrinterAccess.list == NULL)
		return FALSE;

	int idx;
	for (idx=0; idx<G_PrinterAccess.list->len; idx++) {
		data = g_ptr_array_index(G_PrinterAccess.list, idx);
		if (data->isUserID == userB) {
			if ((id > 0 && id == data->id) || (name != NULL && !strcmp(name, data->name)) )
			{
				g_ptr_array_remove_index(G_PrinterAccess.list, idx);
				idx--;
				_free_data(data);
			}
		}
	}

	return TRUE;
}

/**
  @brief  프린터 접근 제어를 할 목록에서 사용자를 제거한다.
  @param	[in]	uid	사용자 ID (리눅스 사용자 ID)
  @return gboolean	성공여부
*/
gboolean grac_apply_printer_access_del_uid  (uid_t uid)
{
	return _printer_access_del_data(TRUE, uid, NULL);
}

/**
  @brief  프린터 접근 제어를 할 목록에서 그룹을 제거한다.
  @param	[in]	gid	그룹 ID (리눅스 그룹 ID)
  @return gboolean	성공여부
*/
gboolean grac_apply_printer_access_del_gid  (uid_t gid)
{
	return _printer_access_del_data(FALSE, gid, NULL);
}

/**
  @brief  프린터 접근 제어를 할 목록에서 사용자를 제거한다.
  @param	[in]	user	사용자명 (리눅스 사용자명)
  @return gboolean	성공여부
*/
gboolean grac_apply_printer_access_del_user (char *user)
{
	return _printer_access_del_data(TRUE, -1, user);
}

/**
  @brief  프린터 접근 제어를 할 목록에서 그룹을 제거한다.
  @param	[in]	group	그룹명 (리눅스 그룹명)
  @return gboolean	성공여부
*/
gboolean grac_apply_printer_access_del_group(char *group)
{
	return _printer_access_del_data(FALSE, -1, group);
}

