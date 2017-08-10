/*
 ============================================================================
 Name        : grac_access_attr.c
 Author      : 
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */

/**
  @file 	 	grac_access_attr.c
  @brief		구름 시스템에서 지원하는 리소스 접근 권한 정보의 리소스명과 permission name 관리 함수들
  @details	헤더파일 :  	grac_access_attr.h	\n
  				라이브러리:	libgrac.so
 */


#include "grac_access.h"
#include "grac_access_attr.h"
#include "grm_log.h"
#include "cutility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

struct _GracAccessAttr
{
	int cur_idx_attr;
	int cur_idx_perm;
	int cur_idx_attr_of_perm;
};
static struct _GracAccessAttr GracAccessAttr
= {
		-1, -1, -1
};
static struct _GracAccessAttr*  pGracAcc = &GracAccessAttr;

struct _GracAccessItem {
	char	*attr_name;		// not use space
	int		attr_id;			//
	struct {
		char	*perm_name;
		int	   perm_value;	// permission
	} item[4];
};


// GUI 도구에 나타나는 순서대로 정의하자
// 처음 데이터가 default
static struct _GracAccessItem GracAccessItem[]
= {
		{ "printer", GRAC_ATTR_PRINTER,  {
				{ "deny",		GRAC_PERM_DENY	 },
				{ "allow",	GRAC_PERM_ALLOW },
				{ NULL, -1 } }
		 },
		{ "network", GRAC_ATTR_NETWORK, {
				{ "deny",		GRAC_PERM_DENY  },
				{	"allow",	GRAC_PERM_ALLOW },
				{ NULL, -1 } }
		 },
		{ "screen-capture", GRAC_ATTR_SCREEN_CAPTURE, {
				{ "deny",		GRAC_PERM_DENY  },
				{	"allow",	GRAC_PERM_ALLOW },
				{ NULL, -1 } }
		 },
		{ "USB", GRAC_ATTR_USB, {
				{ "deny",		GRAC_PERM_DENY  },
				{	"allow",	GRAC_PERM_ALLOW },
				{ NULL, -1 } }
		 },
		{ "Home-Directory",	GRAC_ATTR_HOME_DIR, {
				{ "deny",		GRAC_PERM_DENY  },
				{	"allow",	GRAC_PERM_ALLOW },
				{ NULL, -1 } }
		 },
//		{ "role-setting",  	GRAC_ATTR_ETC+1, {
//				{ "deny",		GRAC_PERM_DENY  },
//				{ "view",		GRAC_PERM_VIEW  },
//				{ "edit",		GRAC_PERM_EDIT	 },
//				{ NULL, -1 } }
//	     	 },
		{ NULL, -1, {{NULL, -1}} }
};

/**
 @brief   지원하는 속성명 나열을 위한 시작 함수
 @details
	  이 후 grac_access_attr_find_next_attr()을 반복적으로 사용하여 속성이름을 구한다.
 @return char* 속성명, 없으면 NULL
 */
char* grac_access_attr_find_first_attr()
{
	int idx = 0;
	for (idx=0; ;idx++) {
		if (GracAccessItem[idx].attr_name == NULL) {
			pGracAcc->cur_idx_attr = -1;
			break;
		}
		pGracAcc->cur_idx_attr = idx;
		return GracAccessItem[idx].attr_name;
	}

	return NULL;
}

/**
 @brief   지원하는 속성명 나열을 위해 또 다른 속성을 가져오는 함수
 @details
	  grac_access_attr_find_next_attr()을 반복적으로 사용하여 속성이름을 구한다.
 @return char* 속성명, 더이상 없으면 NULL
 */
char* grac_access_attr_find_next_attr ()
{
	if (pGracAcc->cur_idx_attr >= 0) {
		int idx = pGracAcc->cur_idx_attr + 1;
		for ( ; ; idx++) {
			if (GracAccessItem[idx].attr_name == NULL) {
				pGracAcc->cur_idx_attr = -1;
				break;
			}
			pGracAcc->cur_idx_attr = idx;
			return GracAccessItem[idx].attr_name;
		}
	}

	return NULL;
}


/**
 @brief   지정된 속성이 지원하는 속성값(Pemission) 나열을 위한 시작 함수
 @details
	  이 후 grac_access_attr_find_next_perm()을 반복적으로 사용하여 속성이름을 구한다.
 @param [in] attr	속성명 (리소스 종류등..)
 @return char*  속성값(Permission Name), 없으면 NULL
 */
char* grac_access_attr_find_first_perm(gchar* attr)
{
	if (attr) {
		int idx = 0;
		for (idx=0; ;idx++) {
			if (GracAccessItem[idx].attr_name == NULL) {
				pGracAcc->cur_idx_attr_of_perm = -1;
				pGracAcc->cur_idx_perm = -1;
				return NULL;
			}
			if (strcmp(attr, GracAccessItem[idx].attr_name)== 0) {
				pGracAcc->cur_idx_attr_of_perm = idx;
				pGracAcc->cur_idx_perm = 0;
				return GracAccessItem[idx].item[0].perm_name;
			}
		}
	}
	return NULL;
}

/**
 @brief   지정된 속성이 지원하는 속성값(Pemission) 나열을 위한 함수
 @return char*  속성값(Permission Name), 없으면 NULL
 */
char* grac_access_attr_find_next_perm ()
{
	if (pGracAcc->cur_idx_attr_of_perm >= 0 && pGracAcc->cur_idx_perm >= 0) {
		int attr_idx = pGracAcc->cur_idx_attr_of_perm;
		int perm_idx = pGracAcc->cur_idx_perm + 1;
		for ( ; ;perm_idx++) {
			if (GracAccessItem[attr_idx].attr_name == NULL
					|| GracAccessItem[attr_idx].item[perm_idx].perm_name == NULL)
			{
				pGracAcc->cur_idx_attr_of_perm = -1;
				pGracAcc->cur_idx_perm = -1;
				break;
			}
			pGracAcc->cur_idx_perm = perm_idx;
			return GracAccessItem[attr_idx].item[perm_idx].perm_name;
		}
	}
	return NULL;

}

/**
 @brief   지정된 속성의 권한값을 구한다.
 @param [in] attr		속성명 (리소스 종류등..)
 @param [in] perm	속성값 (접근권한등..)
 @return int 	권한 값
 */
int	  grac_access_attr_get_perm_value(gchar* attr, gchar *perm)
{
	if (attr && perm) {
		int idx;
		for (idx=0; ;idx++) {
			if (GracAccessItem[idx].attr_name == NULL)
				break;
			if (strcmp(attr, GracAccessItem[idx].attr_name)== 0)
				break;
		}
		int perm_idx;
		for (perm_idx=0; ;perm_idx++) {
			if (GracAccessItem[idx].item[perm_idx].perm_name == NULL)
				break;
			if (strcmp(perm, GracAccessItem[idx].item[perm_idx].perm_name) == 0)
				return GracAccessItem[idx].item[perm_idx].perm_value;
		}
	}

	return -1;
}

/**
 @brief   지정된 속성의 속성값을 구한다.
 @param [in] attr		속성명 (리소스 종류등..)
 @return int 	속성 값
 */
int	  grac_access_attr_get_attr_value(gchar* attr)
{
	if (attr) {
		int idx;
		for (idx=0; ;idx++) {
			if (GracAccessItem[idx].attr_name == NULL)
				break;
			if (strcmp(attr, GracAccessItem[idx].attr_name)== 0)
				return GracAccessItem[idx].attr_id;
		}
	}

	return -1;
}


/**
 @brief   지정된 속성값의 속성명을 구한다.
 @param [in] target		속성값 (리소스 종류등..)
 @return char* 	속성 명
 */
char* grac_access_attr_get_attr_name(int target)
{
	char *name = NULL;
	int idx = 0;
	for (idx=0; ;idx++) {
		if (GracAccessItem[idx].attr_name == NULL) {
			break;
		}
		if (GracAccessItem[idx].attr_id == target) {
			name = GracAccessItem[idx].attr_name;
			break;
		}
	}

	return name;
}

/**
 @brief   지정된 속성,권한의 권한명을 구한다.
 @param [in] target			속성값 (리소스 종류등..)
 @param [in] perm_value	권한값
 @return char* 	권한명
 */
char* grac_access_attr_get_perm_name(int target, int perm_value)
{
	char *name = NULL;
	int idx = 0;

	for (idx=0; ;idx++) {
		if (GracAccessItem[idx].attr_name == NULL) {
			break;
		}
		if (GracAccessItem[idx].attr_id == target) {
			int perm_idx;
			for (perm_idx=0; ;perm_idx++) {
				if (GracAccessItem[idx].item[perm_idx].perm_name == NULL)
					break;
				if (GracAccessItem[idx].item[perm_idx].perm_value == perm_value) {
					name = GracAccessItem[idx].item[perm_idx].perm_name;
					break;
				}
			}
			break;
		}
	}

	return name;
}
