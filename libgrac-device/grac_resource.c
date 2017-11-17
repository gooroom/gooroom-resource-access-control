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
 * grac_resource.c
 *
 *  Created on: 2017. 8. 16.
 *      Author: gooroom@gooroom.kr
 */

/**
  @file 	 	grac_resource.c
  @brief		구름 자원접근 제어 시스템에서 지원하는 자원 관련 정보
  @details	헤더파일 :  	grac_resource.h	\n
  				라이브러리:	libgrac-device.so
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "grac_rule.h"
#include "grac_resource.h"
#include "grac_log.h"
#include "cutility.h"

struct _GracResIter
{
	int cur_idx_res;
	struct _perm {
		int res_id;
		int perm_idx;
	} cur_perm;
	int cur_ctrl_seq;
};

static struct _GracResIter GracResIter
= {
		-1,  { -1,  -1},  -1
};

struct _GracNameIdMap {
	char	*name;
	int		id;
};

// GUI 도구에 나타나는 순서도 동일하게 하자??
static struct _GracNameIdMap GracResourceInfo[]
= {
		{ "network", 		GRAC_RESOURCE_NETWORK 		},
		{ "printer", 		GRAC_RESOURCE_PRINTER 		},
		{ "bluetooth",	GRAC_RESOURCE_BLUETOOTH		},
//	{ "usb",				GRAC_RESOURCE_USB					},
		{ "keyboard",		GRAC_RESOURCE_KEYBOARD	 	},
		{ "mouse",			GRAC_RESOURCE_MOUSE				},
		{ "usb_memory", GRAC_RESOURCE_USB_MEMORY	},
		{ "cd_dvd", 		GRAC_RESOURCE_CD_DVD			},
		{ "screen_capture", GRAC_RESOURCE_S_CAPTURE	},
		{ "clipboard",	GRAC_RESOURCE_CLIPBOARD	},
		{ "microphone",	GRAC_RESOURCE_MICROPHONE	},
		{ "sound",			GRAC_RESOURCE_SOUND			 	},
		{ "camera",			GRAC_RESOURCE_CAMERA			},
		{ "wireless",		GRAC_RESOURCE_WIRELESS		},
		{ "external_lan_card",	GRAC_RESOURCE_EXT_LAN },
		{ "etc",				GRAC_RESOURCE_OTHERS			},
		{ NULL, -1 }
};

// 제어 우선 순위에 맞추자
// 예) usb-storage -> USB
//  usb storage는 USB의  subset이므로 USB 전체 제어는 나중에 수행
static int GracResourceControlSeq[]
= {
		GRAC_RESOURCE_S_CAPTURE		,
		GRAC_RESOURCE_CLIPBOARD		,

		GRAC_RESOURCE_CD_DVD			,
		GRAC_RESOURCE_MICROPHONE	,
		GRAC_RESOURCE_SOUND			 	,
		GRAC_RESOURCE_CAMERA			,

		GRAC_RESOURCE_PRINTER 		,
		GRAC_RESOURCE_KEYBOARD	 	,
		GRAC_RESOURCE_MOUSE				,

		GRAC_RESOURCE_EXT_LAN 	  ,
		GRAC_RESOURCE_WIRELESS		,
		GRAC_RESOURCE_NETWORK 		,
		GRAC_RESOURCE_BLUETOOTH		,
		GRAC_RESOURCE_USB_MEMORY	,
		GRAC_RESOURCE_USB					,

		-1
};


static struct _GracNameIdMap GracPemissionInfo[]
= {
		{ "disallow", 	GRAC_PERMISSION_DISALLOW  },
		{ "allow",			GRAC_PERMISSION_ALLOW			},
		{ "read_only", 	GRAC_PERMISSION_READONLY  },
		{ "read_write",	GRAC_PERMISSION_READWRITE },
		{ NULL, -1 }
};

static int get_id(char *name, struct _GracNameIdMap *map, int count)
{
	int id = -1;

	if (name && map) {
		int idx;
		for (idx=0; idx < count; idx++) {
			if (map->name == NULL)
				break;
			if (c_strimatch(name, map->name)) {
				id = map->id;
				break;
			}
			map++;
		}
	}

	return id;
}

static char* get_name(int id, struct _GracNameIdMap *map, int count)
{
	char* name = NULL;

	if (map) {
		int idx;
		for (idx=0; idx < count; idx++) {
			if (map->name == NULL)
				break;
			if (id == map->id) {
				name = map->name;
				break;
			}
			map++;
		}
	}
	return name;
}

/**
 @brief   지정된 리소스ID의 리소스명을 구한다.
 @param [in] res_id		리소스 ID
 @return char* 	리소스명
 */

char* grac_resource_get_resource_name(int res_id)
{
	int count = sizeof(GracResourceInfo) / sizeof(GracResourceInfo[0]);
	return get_name(res_id, GracResourceInfo, count);
}

/**
 @brief   지정된 권한의 권한명을 구한다.
 @param [in] perm_id		권한ID
 @return char* 	권한명
 */
char* grac_resource_get_permission_name(int perm_id)
{
	int count = sizeof(GracPemissionInfo) / sizeof(GracPemissionInfo[0]);
	return get_name(perm_id, GracPemissionInfo, count);
}

/**
 @brief   지정된 리소스의 ID를 구한다.
 @param [in] res_name		리소스 명
 @return int 	리소스ID
 */
int		grac_resource_get_resource_id(char* res_name)
{
	int count = sizeof(GracResourceInfo) / sizeof(GracResourceInfo[0]);
	return get_id(res_name, GracResourceInfo, count);
}

/**
 @brief   지정된 권한의 ID를 구한다.
 @param [in] perm_name	권한명
 @return int 	권한 ID
 */
int		grac_resource_get_permission_id(char* perm_name)
{
	int count = sizeof(GracPemissionInfo) / sizeof(GracPemissionInfo[0]);
	return get_id(perm_name, GracPemissionInfo, count);
}

/**
 @brief   지원하는 리소스 명 나열을 위한 시작 함수
 @details
	  이 후 grac_resource_find_next_resource()을 반복적으로 사용하여 리소스 이름을 구한다.
 @return char* 리소스명, 없으면 NULL
 */
char* grac_resource_find_first_resource()
{
	char	*name = NULL;
	int idx = 0;

	if (GracResourceInfo[idx].name == NULL) {
		GracResIter.cur_idx_res = -1;
	} else {
		GracResIter.cur_idx_res = idx;
		name = GracResourceInfo[idx].name;
	}

	return name;
}

/**
 @brief   지원하는 리소스 명 나열을 위해 다음 리소스명을 가져오는 함수
 @details
	  grac_resource_find_next_resource()을 반복적으로 사용하여 리소스 이름을 구한다.
 @return char* 리소스명, 더이상 없으면 NULL
 */
char* grac_resource_find_next_resource()
{
	char *name = NULL;

	if (GracResIter.cur_idx_res >= 0) {
		int idx = GracResIter.cur_idx_res + 1;

		if (GracResourceInfo[idx].name == NULL) {
			GracResIter.cur_idx_res = -1;
		} else {
			GracResIter.cur_idx_res = idx;
			name = GracResourceInfo[idx].name;
		}
	}

	return name;
}

/**
 @brief   지정된 리소스에 사용되는 권한명 나열을 위한 시작 함수
 @details
	  이 후 grac_resource_find_next_permission()을 반복적으로 사용하여 권한이름을 구한다.
 @param [in] res_id	리소스 종류
 @return char*  권한값, 없으면 NULL
 */
char* grac_resource_find_first_permission(int res_id)
{
	char *name = NULL;
	int idx_res = -1;
	int idx = 0;
	int	count;

	count = sizeof(GracResourceInfo) / sizeof(GracResourceInfo[0]);
	for (idx=0; idx < count ; idx++) {
		if (GracResourceInfo[idx].id == res_id) {
			idx_res = idx;
			break;
		}
		if (GracResourceInfo[idx].name == NULL)
			break;
	}

	if (idx_res == -1) {
		GracResIter.cur_perm.res_id = -1;
		GracResIter.cur_perm.perm_idx = -1;
	} else {
		GracResIter.cur_perm.res_id = res_id;
		GracResIter.cur_perm.perm_idx = 0;
		name = grac_resource_get_permission_name(GRAC_PERMISSION_DISALLOW);
	}
	return name;
}

/**
 @brief   지정된 리소스에 사용되는 권한명(Pemission) 나열을 위한 함수
 @details
	  grac_resource_find_next_permission()을 반복적으로 사용하여 권한이름을 구한다.
 @return char*  권한명, 없으면 NULL
 Todo:
 	 경우수사 많아지면 테이블로 구현
 */
char* grac_resource_find_next_permission()
{
	char *name = NULL;

	if (GracResIter.cur_perm.res_id >= 0 && GracResIter.cur_perm.perm_idx >= 0) {
		if (grac_resource_is_kind_of_storage(GracResIter.cur_perm.res_id)) {
				switch (GracResIter.cur_perm.perm_idx) {
				case 0:
					GracResIter.cur_perm.perm_idx++;
					name = grac_resource_get_permission_name(GRAC_PERMISSION_READONLY);
					break;
				case 1:
					GracResIter.cur_perm.perm_idx++;
					name = grac_resource_get_permission_name(GRAC_PERMISSION_READWRITE);
					break;
				default:
					GracResIter.cur_perm.res_id = -1;
					GracResIter.cur_perm.perm_idx = -1;
					break;
				}
		} else {
				switch (GracResIter.cur_perm.perm_idx) {
				case 0:
					GracResIter.cur_perm.perm_idx++;
					name = grac_resource_get_permission_name(GRAC_PERMISSION_ALLOW);
					break;
				default:
					GracResIter.cur_perm.res_id = -1;
					GracResIter.cur_perm.perm_idx = -1;
					break;
				}
		}
	}

	return name;
}


gboolean grac_resource_is_kind_of_storage(int res_id)
{
	if (res_id == GRAC_RESOURCE_USB_MEMORY ||
			res_id == GRAC_RESOURCE_CD_DVD)
    {
			return TRUE;
	}

	return FALSE;
}

int	grac_resource_first_control_res_id()
{
	GracResIter.cur_ctrl_seq = 0;
	return GracResourceControlSeq[0];
}

int	grac_resource_next_control_res_id()
{
	int res_id = -1;

	if (GracResIter.cur_ctrl_seq >= 0) {
		int cnt = sizeof(GracResourceControlSeq) / sizeof(GracResourceControlSeq[0]);
		GracResIter.cur_ctrl_seq++;
		if (GracResIter.cur_ctrl_seq >= cnt)
			GracResIter.cur_ctrl_seq = -1;
		else if (GracResourceControlSeq[GracResIter.cur_ctrl_seq] >= 0)
			res_id = GracResourceControlSeq[GracResIter.cur_ctrl_seq];
		else
			GracResIter.cur_ctrl_seq = -1;
	}

	return res_id;
}
