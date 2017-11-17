/*
 * Copyright (c) 2015 - 2017 gooroom <gooroom@gooroom.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * grac_rule_printer_cups.c
 *
 *  Created on: 2017. 10. 23.
 *      Author: gooroom@gooroom.kr
*/


/**
  @file 	 	grac_rule_printer_cups.c
  @brief		CUPS를 이용한 프린터 접근 제어
  @details	class GracRulePrinterCups 정의 (GTK style의 c언어를 이용한 객체처리 사용) \n
  				헤더파일 :  	grac_rule_printer_cups.h	\n
  				라이브러리:	libgrac-device.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "grac_rule_printer_cups.h"
#include "grac_log.h"
#include "cutility.h"
#include "sys_cups.h"
#include "sys_user.h"

/*
 *  Printer
 *       - one time operation for all user or groups
 */

typedef struct _GrmPrinterAccData GrmPrinterAccData;
struct _GrmPrinterAccData {
	gboolean allow;
	gboolean isUserID;
	int		   id;
	char	   *name;
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
		c_memset(data, 0, sizeof(GrmPrinterAccData));
		data->name = malloc(nameSize+1);
		if (data->name == NULL) {
			free(data);
			data = NULL;
		} else {
			*data->name = 0;
		}
	}
	return data;
}

static void _free_data(GrmPrinterAccData* data)
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
		for (i=0; i < G_PrinterAccess.list->len; i++) {
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
			res = grac_rule_printer_cups_init();
			res = grac_rule_printer_cups_add_uid  (uid, TRUE); 	// 설정하고자하는 사용자 id별로 반복 호출
			res = grac_rule_printer_cups_apply(TRUE);				// 사용자 혹은 그룹 등록이 완료되면 시스템으로의 반영을 요청한다.
			grac_rule_printer_cups_clear();
  @endcode
  @return gboolean	성공여부
*/
gboolean grac_rule_printer_cups_init()
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
void  grac_rule_printer_cups_final()
{
	_free_buf();

	sys_cups_access_final();
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
gboolean grac_rule_printer_cups_apply(gboolean allow)
{
	if (G_PrinterAccess.list == NULL)
		return FALSE;

	int	i;
	gboolean res = TRUE;

	for (i=0; i < G_PrinterAccess.list->len; i++) {
		GrmPrinterAccData* data;
		data = g_ptr_array_index(G_PrinterAccess.list, i);
		if (data->allow == allow) {
			if (data->isUserID == TRUE) {
				res &= sys_cups_access_add_user(data->name, allow);
			} else {
				res &= sys_cups_access_add_group(data->name, allow);
			}
		}
	}
	if (res == TRUE)
		res = sys_cups_access_apply(allow);

	return res;
}

static gboolean _printer_access_add_data(gboolean userB, int id, char* name, gboolean allow)
{
	GrmPrinterAccData* data;
	data = _alloc_data(c_strlen(name, 256)+1);
	if (data != NULL) {
		data->isUserID = userB;
		data->id = id;
		c_strcpy(data->name, name, 256);
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
gboolean grac_rule_printer_cups_add_uid(uid_t uid, gboolean allow)
{
	gboolean res;
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
gboolean grac_rule_printer_cups_add_gid(uid_t gid, gboolean allow)
{
	gboolean res;
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
gboolean grac_rule_printer_cups_add_user(char *user,  gboolean allow)
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
gboolean grac_rule_printer_cups_add_group(char *group, gboolean allow)
{
	gboolean res = FALSE;
	gid_t gid = sys_user_get_gid_from_name(group);
	if (gid > 0)
		res = _printer_access_add_data(FALSE, gid, group, allow);

	return res;
}

static gboolean _printer_access_del_data(gboolean userB, int id, char* name)
{
	if (G_PrinterAccess.list == NULL)
		return FALSE;

	int idx;
	for (idx=0; idx < G_PrinterAccess.list->len; idx++) {
		GrmPrinterAccData* data = g_ptr_array_index(G_PrinterAccess.list, idx);
		if (data->isUserID == userB) {
			if ((id > 0 && id == data->id) || (name != NULL && c_strmatch(name, data->name)) )
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
gboolean grac_rule_printer_cups_del_uid(uid_t uid)
{
	return _printer_access_del_data(TRUE, uid, NULL);
}

/**
  @brief  프린터 접근 제어를 할 목록에서 그룹을 제거한다.
  @param	[in]	gid	그룹 ID (리눅스 그룹 ID)
  @return gboolean	성공여부
*/
gboolean grac_rule_printer_cups_del_gid(uid_t gid)
{
	return _printer_access_del_data(FALSE, gid, NULL);
}

/**
  @brief  프린터 접근 제어를 할 목록에서 사용자를 제거한다.
  @param	[in]	user	사용자명 (리눅스 사용자명)
  @return gboolean	성공여부
*/
gboolean grac_rule_printer_cups_del_user(char *user)
{
	return _printer_access_del_data(TRUE, -1, user);
}

/**
  @brief  프린터 접근 제어를 할 목록에서 그룹을 제거한다.
  @param	[in]	group	그룹명 (리눅스 그룹명)
  @return gboolean	성공여부
*/
gboolean grac_rule_printer_cups_del_group(char *group)
{
	return _printer_access_del_data(FALSE, -1, group);
}

