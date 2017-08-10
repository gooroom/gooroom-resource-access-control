/*
 * user_list.c
 *
 *  Created on: 2016. 4. 12.
 *      Author: user
 */

/**
  @file 	 	sys_user_list.c
  @brief		 리눅스 시스템의 사용자 목록 관리
  @details	class SysUserList 정의 (GTK style의 c언어를 이용한 객체처리 사용) \n
  				헤더파일 :  	sys_user_list.h	\n
  				라이브러리:	libgrac.so
 */


#include "sys_user.h"
#include "sys_user_list.h"

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

struct sys_user_list_data {
	gchar *name;
	int id;
} ;

struct sys_user_list {
	GPtrArray *data;
	int	count;
	int	kind;
};

/**
 @brief   sys_user_list  객체 생성
 @details
      	객체 지향형 언어의 생성자에 해당한다.	\n
		시스템 상황이 변경되는 경우가 있고		\n
		함수 getpwent()가 thread-safe 하지않기 때문에 미리 구해놓는다.
 @param [in] kind		사용자 유형 (SYS_USER_KIND_GENERAL,  SYS_USER_KIND_SYSTEM)
 @return 생성된 객체 주소
 */
struct sys_user_list* sys_user_list_alloc(int kind)
{
	struct sys_user_list* plist;

	plist = (struct sys_user_list*)malloc(sizeof(struct sys_user_list));
	if (plist == NULL)
		return NULL;

	memset(plist, 0, sizeof(struct sys_user_list));
	plist->data = g_ptr_array_new();
	if (plist->data == NULL) {
		free(plist);
		return NULL;
	}

	int uid_min, uid_max;
	int sys_uid_min, sys_uid_max;
	sys_user_get_uid_range(&uid_min, &uid_max);
	sys_user_get_sys_uid_range(&sys_uid_min, &sys_uid_max);

	struct passwd *pwd;
	setpwent();
	plist->count = 0;

	pwd = getpwent();
	while (pwd) {
		gboolean addB = FALSE;
		if (kind == SYS_USER_KIND_ALL) {	// to include ids being out of range
			addB = TRUE;
		}
		else {
			if (kind & SYS_USER_KIND_GENERAL) {
				if (pwd->pw_uid >= uid_min && pwd->pw_uid <= uid_max)
					addB = TRUE;
			}
			if (kind & SYS_USER_KIND_SYSTEM) {
				if (pwd->pw_uid >= sys_uid_min && pwd->pw_uid <= sys_uid_max)
					addB = TRUE;
			}
		}
		if (addB) {
			struct sys_user_list_data* pdata;
			pdata = malloc(sizeof(struct sys_user_list_data));
			if (pdata) {
				pdata->name = malloc(strlen(pwd->pw_name)+1);
				if (pdata->name)
					strcpy(pdata->name, pwd->pw_name);
				pdata->id = pwd->pw_uid;
				g_ptr_array_add (plist->data, (gpointer)pdata);
				plist->count++;
			}
		}
		pwd = getpwent();
	}
	endpwent();

	return plist;
}

/**
 @brief   sys_user_list 객체 해제
 @details
      객체 지향형 언어의 소멸자에  해당한다. 단 자동으로 불려지지 않고 필요한 시점에서 직접 호출하여야 한다.\n
      해제된  객체 주소값으로 0으로 설정하기 위해 이중 포인터를 사용한다.
 @param [in,out]  list  해제하고자 하는 객체 주소를 담는 변수의 주소 (double pointer)
 */
void   sys_user_list_free(struct sys_user_list** list)
{
	if (list != NULL) {
		struct sys_user_list *plist = *list;
		if (plist) {
			int idx;
			for (idx=0; idx<plist->count; idx++) {
				struct sys_user_list_data *pdata;
				pdata = g_ptr_array_index (plist->data, idx);
				if (pdata != NULL) {
					if (pdata->name != NULL)
						free(pdata->name);
					free(pdata);
				}
			}
			if (plist->data != NULL)
				g_ptr_array_free(plist->data, TRUE);
			free(plist);
		}
		*list = NULL;
	}
}

/**
  @brief   사용자 갯수를 구한다.
  @details
  		일단 만들어진 목록에는 시스템의 변화가 반영되지 않는다.
  @param [in]  plist	sys_user_lsit 객체주소
  @return int  사용자 수
 */
int	   sys_user_list_count(struct sys_user_list* plist)
{
	int	count = 0;
	if (plist)
		count = plist->count;
	return count;
}

/**
  @brief   지정된 위치의 사용자 이름을 구한다.
  @details
  		일단 만들어진 목록에는 시스템의 변화가 반영되지 않는다.
  @param [in]  plist	sys_user_lsit 객체주소
  @param [in]  idx	    사용자 위치 : 0에서 갯수 -1까지 유효
  @return char*  사용자이름
 */
char*  sys_user_list_get_name(struct sys_user_list* plist, int idx)
{
	if (plist == NULL)
		return NULL;
	if (idx < 0 || idx >= plist->count)
		return NULL;

	struct sys_user_list_data *pdata;
	char	*name = NULL;
	pdata = g_ptr_array_index (plist->data, idx);
	if (pdata != NULL) {
		if (pdata->name != NULL)
			name = pdata->name;
	}

	return name;
}

// if error, return -1
int  sys_user_list_get_uid (struct sys_user_list* plist, int idx)
{
	int id = -1;

	if (plist != NULL) {
		if (idx >=0 && idx < plist->count) {
			struct sys_user_list_data *pdata;
			pdata = g_ptr_array_index (plist->data, idx);
			if (pdata != NULL) {
				id = pdata->id;
			}
		}
	}
	return id;
}

/**
  @brief   사용자가 존재하는지 확인한다.
  @details
  		일단 만들어진 목록에는 시스템의 변화가 반영되지 않는다.
  @param [in]  plist			sys_user_lsit 객체주소
  @param [in]  find_name 	찾고자하는 사용자 이름
  @return int   사용자 위치,  없는 경우 -1
 */
int	sys_user_list_find_name(struct sys_user_list* plist, char *find_name)
{
	int	 count, idx = -1;
	int	 i;
	char *tmp_name;

	count = sys_user_list_count(plist);
	for (i=0; i<count; i++) {
		tmp_name = sys_user_list_get_name(plist, i);
		if (tmp_name && strcmp(tmp_name, find_name) == 0) {
			idx = i;
			break;
		}
	}

	return idx;
}

/**
  @brief   사용자가 존재하는지 확인한다.
  @details
  		일단 만들어진 목록에는 시스템의 변화가 반영되지 않는다.
  @param [in]  plist			sys_user_lsit 객체주소
  @param [in]  find_uid	 	찾고자하는 사용자 ID
  @return int   사용자 위치,  없는 경우 -1
 */
int	sys_user_list_find_uid (struct sys_user_list* plist, int find_uid)
{
	int	 count, idx = -1;
	int	 i;
	int  tmp_uid;

	count = sys_user_list_count(plist);
	for (i=0; i<count; i++) {
		tmp_uid = sys_user_list_get_uid(plist, i);
		if (tmp_uid == find_uid) {
			idx = i;
			break;
		}
	}

	return idx;
}
