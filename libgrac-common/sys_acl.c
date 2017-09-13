/*
 * sys_acl.c
 *
 *  Created on: 2016. 1. 20.
 *      Author: user
 */

/**
  @file 	 	sys_acl.c
  @brief		리눅스의 ACL (Access Control List) 설정 관련 함수
  @details	시스템 혹은 특정 라이브러리 의존적인 부분을 분리하는 목적의 래퍼 함수들이다.  \n
  				현재는 libacl을 이용하여 구현되었다.	\n
  				헤더파일 :  	sys_acl.h	\n
  				라이브러리:	libgrac.so
 */


#include "sys_acl.h"
#include "sys_user.h"
#include "grm_log.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <acl/libacl.h>

/*
 *  2016.5.  update by using libacl
 */

static gboolean g_recalcMask = TRUE;

/**
  @brief   ACL 설정후에  설절에 맞게 mask를 재설정 할 것인지를 지정한다.
  @details
  		기본으로 TRUE이며 TRUE인 경우 모든 ACL 설정 함수 호출시  mask 재설정이 이루어진다.
  @param [in]  recalcB	재설정여부
  */
void sys_acl_set_recalc_mask(gboolean recalcB)		// 2016.5  default mode = TRUE
{
	g_recalcMask = recalcB;
}

static void _do_recalc_mask(acl_t *acl_p)
{
	if (g_recalcMask == TRUE)
		acl_calc_mask(acl_p);
}

// 2016.5
/**
  @brief   설정되어 있는 모든 ACL entry를 제거한다.
  @details
  		제거되는 entry에는  default entry도 포함된다.
  @param [in]  path	파일 또는 디렉토리 경로
  @return gboolean 성공 여부
  */
gboolean sys_acl_clear(gchar *path)
{
	struct stat sb;
	int	res;
	gboolean retB = TRUE;

	res = stat(path, &sb);
	if (res == -1) {
		grm_log_error("Error: sys_acl_clear: %s (%s)", strerror(errno), path);
		return FALSE;
	}

	if (S_ISDIR(sb.st_mode)) {
		retB = sys_acl_clear_by_type(path, ACL_TYPE_DEFAULT);
	}

	retB &= sys_acl_clear_by_type(path, ACL_TYPE_ACCESS);

	return retB;

#if 0
	gchar cmd[1024];

	sprintf(cmd, "setfacl -b %s  2>&1", path);

	gboolean ret;
	FILE *fp;

	fp = popen(cmd, "r");
	if (fp != NULL) {
		ret = TRUE;
//	char buf[1024];
//	while (1) {
//	 	if (fgets(buf, sizeof(buf), fp) == NULL)
//			break;
//	 	ret = FALSE;		// may be error message
//		}
		pclose(fp);
	}
	else {
		ret = FALSE;
	}
	return ret;
#endif

}

/**
  @brief   설정되어 있는 ACL entry 중에서 지정한 타입의 entry를 제거한다.
  @details
  		ACL type은 ACL_TYPE_DEFAULT, ACL_TYPE_ACCESS 가 있다.
  @param [in]  path	파일 또는 디렉토리 경로
  @param [in]  type	ACL type
  @return gboolean 성공 여부
 */
gboolean sys_acl_clear_by_type(gchar *path, acl_type_t type)
{
	gboolean retB = TRUE;
	int	res;

	if (type == ACL_TYPE_DEFAULT) {
		res = acl_delete_def_file(path);
		if (res == -1) {
			grm_log_error("Error: sys_acl_clear: %s (%s)", strerror(errno), path);
			retB = FALSE;
		}
	}
	else {  // ACL_TYPE_ACCESS
		acl_t acl;
		acl = acl_get_file(path, type);
		if (acl == NULL) {
			grm_log_error("Error: sys_acl_clear: %s (%s)", strerror(errno), path);
			retB = FALSE;
		}
		else {
			acl_free(acl);
			acl = acl_init(0);
			res = acl_set_file(path, type, acl);
			if (res == -1) {
				grm_log_error("Error: sys_acl_clear: %s (%s)", strerror(errno), path);
				retB = FALSE;
			}
		}
	}

	return retB;

}

// entry 검색
static acl_entry_t _acl_find_entry(acl_t acl, acl_tag_t tag, id_t qaul)
{
    acl_entry_t entry;
    acl_tag_t entryTag;
    uid_t *uidp;
    gid_t *gidp;
    int ent, s;

    for (ent = ACL_FIRST_ENTRY; ; ent = ACL_NEXT_ENTRY) {
        s = acl_get_entry(acl, ent, &entry);
        if (s == -1) {
        	grm_log_error("Error: acl_find_entry: %s", strerror(errno));
          return NULL;
    			}
        if (s == 0) {  // no more
            return NULL;
        		}

        if (acl_get_tag_type(entry, &entryTag) == -1) {
        	grm_log_error("Error: acl_find_entry: %s", strerror(errno));
        	continue;
        		}

        if (tag != entryTag) {
        	continue;
        		}

				if (tag == ACL_USER) {
						uidp = acl_get_qualifier(entry);
						if (uidp == NULL) {
							grm_log_error("Error: acl_find_entry: %s", strerror(errno));
						}
						else if (qaul == *uidp) {
							acl_free(uidp);
							return entry;
						}
						else {
							acl_free(uidp);
						}
				}
			  else if (tag == ACL_GROUP) {
						gidp = acl_get_qualifier(entry);
						if (gidp == NULL) {
							grm_log_error("Error: acl_find_entry: %s", strerror(errno));
						}
						else if (qaul == *gidp) {
							acl_free(gidp);
							return entry;
						}
						else {
							acl_free(gidp);
						}
			  	}
			  else {
						return entry;
				}
    	}
    return NULL;
}

// 퍼미션 설정
// perms : ACL_READ, ACL_WRITE, ACL_EXECUTE  (bitwise)
static gboolean _acl_set_perms(acl_entry_t entry, int perms)
{
		gboolean  resB = TRUE;
    acl_permset_t permset;

    if (acl_get_permset(entry, &permset) == -1) {
    		grm_log_error("Error: acl_set_perms: %s", strerror(errno));
        resB = FALSE;
		}

    if (acl_clear_perms(permset) == -1) {
    	grm_log_error("Error: acl_set_perms: %s", strerror(errno));
    	resB = FALSE;
		}

    if (perms & ACL_READ) {
        if (acl_add_perm(permset, ACL_READ) == -1) {
        	grm_log_error("Error: acl_set_perms: %s", strerror(errno));
        	resB = FALSE;
        		}
    	}
    if (perms & ACL_WRITE) {
        if (acl_add_perm(permset, ACL_WRITE) == -1) {
        	grm_log_error("Error: acl_set_perms: %s", strerror(errno));
        	resB = FALSE;
        		}
        }

    if (perms & ACL_EXECUTE) {
        if (acl_add_perm(permset, ACL_EXECUTE) == -1) {
        	grm_log_error("Error: acl_set_perms: %s", strerror(errno));
        	resB = FALSE;
        		}
    	}

    if (acl_set_permset(entry, permset) == -1) {
    	grm_log_error("Error: acl_set_perms: acl_set_permset() - %s", strerror(errno));
    	resB = FALSE;
    	}

    return resB;
}

//  default ACL entry 설정
//  tag: ACL_USER_OBJ(파일소유자), ACL_GROUP_OBJ(파일그룹), ACL_OTHER
//  default 설정이므로 상기 tag만 유효하다.
//  파일의 기본 사용자, 그룹 외에 추가되는 것은 ACL_USER, ACL_GROUP
static gboolean _acl_default_basic_entry(char *path, acl_t default_acl, acl_tag_t tag)
{
	struct stat sb;
	gboolean resB = TRUE;

	if (stat(path, &sb) == -1) {
		grm_log_error("Error: acl_default_basic_entry: stat() - %s (%s)", strerror(errno), path);
		return FALSE;
	}

	acl_t basic_acl;

	basic_acl = acl_from_mode(sb.st_mode);	// file의 기본 permission으로 부터 ACL을 구한다.
	if (basic_acl == NULL) {
		grm_log_error("Error: acl_default_basic_entry: acl_from_mode() - %s (%s)", strerror(errno), path);
		return FALSE;
	}

	acl_entry_t  from_entry, to_entry;
	from_entry = _acl_find_entry(basic_acl, tag, ACL_UNDEFINED_ID);    // 파일의 기본 퍼미션
	to_entry = _acl_find_entry(default_acl, tag, ACL_UNDEFINED_ID);     // default ACL이 있는지 확인
	if (from_entry) {
		if (to_entry == NULL) {  // default  ACL에 없는 경우에 실행
			if (acl_create_entry(&default_acl, &to_entry) == -1) {  // 생성
					grm_log_error("Error: acl_default_basic_entry: acl_create_entry() - %s (%s)", strerror(errno), path);
					resB = FALSE;
			}
			else {  // file의 기본 permission으로부터 구한 값을 default ACL에 설정한다.
				acl_copy_entry(to_entry, from_entry);
				acl_set_tag_type(to_entry, ACL_TYPE_DEFAULT);
			}
		}
	}
	else {
		resB = FALSE;
	}

	acl_free(basic_acl);

	return resB;
}


/**
  @brief   ACL 퍼미션을 설정한다.
  @details
  		해당 entry가 없는 경우는 생성한다.
  @param [in]  path	파일 또는 디렉토리 경로
  @param [in]  id		user id 혹은 group id 혹은 ACL_UNDEFINED_ID
  @param [in]  type	ACL type	(ACL_TYPE_DEFAULT, ACL_TYPE_ACCESS)
  @param [in]  tag		ACL tag    (ACL_USER, ACL_USER_OBJ,.....)
  @param [in]  perm	ACL permission (ACL_READ....)
  @return gboolean 성공 여부
 */
gboolean sys_acl_set_by_id(gchar *path, id_t id, acl_type_t type, acl_tag_t tag, acl_perm_t perm)
{
		gboolean resB = TRUE;
    acl_t acl;
    acl_entry_t entry;

    acl = acl_get_file(path, type);
    if (acl == NULL) {
    	grm_log_error("Error: sys_acl_set_by_id: %s (%s)", strerror(errno), path);
    	return FALSE;
    	}

		entry = _acl_find_entry(acl, tag, id);

		if (entry == NULL) {
				if (acl_create_entry(&acl, &entry) == -1) {
						grm_log_error("Error: sys_acl_set_by_id: acl_create_entry() - %s (%s)", strerror(errno), path);
						resB = FALSE;
				}
				if (resB) {
						if (acl_set_tag_type(entry, tag) == -1) {
							grm_log_error("Error: sys_acl_set_by_id: acl_set_tag_type() - %s (%s)", strerror(errno), path);
							resB = FALSE;
						}
				}
				if (resB && (tag == ACL_USER || tag == ACL_GROUP)) {
						if (acl_set_qualifier(entry, &id) == -1) {
							grm_log_error("Error: sys_acl_set_by_id: acl_set_qualifier() - %s (%s)", strerror(errno), path);
							resB = FALSE;
						}
				}
		}

		if (resB) {
			_acl_set_perms(entry, perm);
		}

		if (resB && type == ACL_TYPE_DEFAULT) {
			entry = _acl_find_entry(acl, ACL_USER_OBJ, ACL_UNDEFINED_ID);
			if (entry == NULL) {
				resB &= _acl_default_basic_entry(path, acl, ACL_USER_OBJ);
			}
			entry = _acl_find_entry(acl, ACL_GROUP_OBJ, ACL_UNDEFINED_ID);
			if (entry == NULL) {
				resB &= _acl_default_basic_entry(path, acl, ACL_GROUP_OBJ);
			}
			entry = _acl_find_entry(acl, ACL_OTHER, ACL_UNDEFINED_ID);
			if (entry == NULL) {
				resB &= _acl_default_basic_entry(path, acl, ACL_OTHER);
			}
		}

		if (resB) {
			_do_recalc_mask(&acl);
		}

		if (resB) {
				if (acl_valid(acl) == -1) {
					grm_log_error("Error: sys_acl_set_by_id: acl_valid() - %s (%s)", strerror(errno), path);
					resB = FALSE;
				}
		}

		if (resB) {
				if (acl_set_file(path, type, acl) == -1) {
						grm_log_error("Error: sys_acl_set_by_id: acl_set_file() - %s (%s)", strerror(errno), path);
				    resB = FALSE;
				}
		}

		acl_free(acl);

		return resB;
}

/**
  @brief   설정되어 있는 ACL 퍼미션을 구한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   id			user id 혹은 group id 혹은 ACL_UNDEFINED_ID
  @param [in]   type		ACL type	(ACL_TYPE_DEFAULT, ACL_TYPE_ACCESS)
  @param [in]   tag		ACL tag    (ACL_USER, ACL_USER_OBJ,.....)
  @param [out] perm	ACL permission (ACL_READ....)
  @return gboolean 성공 여부
 */
gboolean sys_acl_get_by_id(gchar *path, id_t id, acl_type_t type, acl_tag_t tag, acl_perm_t *perm)
{
	gboolean resB = TRUE;
	acl_t acl;
	acl_entry_t entry;

	*perm = 0;

	acl = acl_get_file(path, type);
	if (acl == NULL) {
		grm_log_error("Error: sys_acl_get_by_id: %s (%s)", strerror(errno), path);
		return FALSE;
	}

	entry = _acl_find_entry(acl, tag, id);

	if (entry == NULL) {
		grm_log_debug("Debug: sys_acl_get_by_id: not found entry");
		resB = FALSE;
	}
	else {
    acl_permset_t permset;

		if (acl_get_permset(entry, &permset) == -1) {
			grm_log_error("Error: sys_acl_get_by_id: acl_get_permset() - %s (%s)", strerror(errno), path);
			resB = FALSE;
		}
		else {
			int set;
			set = acl_get_perm(permset, ACL_READ);
			if (set == -1) {
				grm_log_error("Error: sys_acl_get_by_id: acl_get_perm() - %s (%s)", strerror(errno), path);
				resB = FALSE;
			}
			else if (set == 1) {
				*perm |= ACL_READ;
			}
			set = acl_get_perm(permset, ACL_WRITE);
			if (set == -1) {
				grm_log_error("Error: sys_acl_get_by_id: acl_get_perm() - %s (%s)", strerror(errno), path);
				resB = FALSE;
			}
			else if (set == 1) {
				*perm |= ACL_WRITE;
			}
			set = acl_get_perm(permset, ACL_EXECUTE);
			if (set == -1) {
				grm_log_error("Error: sys_acl_get_by_id: acl_get_perm() - %s (%s)", strerror(errno), path);
				resB = FALSE;
			}
			else if (set == 1) {
				*perm |= ACL_EXECUTE;
			}
		}
	}

	acl_free(acl);

	return resB;

}

/**
  @brief   지정된 ACL 퍼미션을 제거한다.
  @details
  		해당 entry가 없는 경우는 오류로 처리한다..
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   id			user id 혹은 group id 혹은 ACL_UNDEFINED_ID
  @param [in]   type		ACL type	(ACL_TYPE_DEFAULT, ACL_TYPE_ACCESS)
  @param [in]   tag		ACL tag    (ACL_USER, ACL_USER_OBJ,.....)
  @return gboolean 성공 여부
 */
gboolean sys_acl_del_by_id(gchar *path, id_t id, acl_type_t type, acl_tag_t tag)
{
    acl_t acl;
    acl_entry_t entry;
   	gboolean resB = TRUE;

    acl = acl_get_file(path, type);
    if (acl == NULL) {
    	grm_log_error("Error: sys_acl_set_by_id: %s (%s)", strerror(errno), path);
    	return FALSE;
    	}

		entry = _acl_find_entry(acl, tag, id);
		if (entry != NULL) {
			if (acl_delete_entry(acl, entry) == -1) {
				grm_log_error("Error: sys_acl_del_by_id: acl_delete_entry() - %s (%s)", strerror(errno), path);
				resB = FALSE;
			}

			if (resB) {
				_do_recalc_mask(&acl);
			}

			if (resB) {
					if (acl_valid(acl) == -1) {
						grm_log_error("Error: sys_acl_del_by_id: invalid acl");
						resB = FALSE;
					}
			}

			if (resB) {
					if (acl_set_file(path, type, acl) == -1) {
							grm_log_error("Error: sys_acl_del_by_id: acl_set_file() - %s (%s)", strerror(errno), path);
					    resB = FALSE;
					}
			}

		}

		acl_free(acl);

		return resB;
}

/**
  @brief   ACL 퍼미션을 설정한다.
  @details
  		해당 entry가 없는 경우는 생성한다.
  @param [in]  path	파일 또는 디렉토리 경로
  @param [in]  name	user name 혹은 group name
  @param [in]  type	ACL type	(ACL_TYPE_DEFAULT, ACL_TYPE_ACCESS)
  @param [in]  tag		ACL tag    (ACL_USER, ACL_USER_OBJ,.....)
  @param [in]  perm	ACL permission (ACL_READ....)
  @return gboolean 성공 여부
 */
gboolean sys_acl_set_by_name(gchar *path, char *name, acl_type_t type, acl_tag_t tag, acl_perm_t perm)
{
	id_t id = -1;

	if (tag == ACL_USER)
		id = sys_user_get_uid_from_name(name);
	else if (tag == ACL_GROUP)
		id = sys_user_get_gid_from_name(name);

	return sys_acl_set_by_id(path, id, type, tag, perm);
}

/**
  @brief   설정되어 있는 ACL 퍼미션을 구한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   name	user name 혹은 group name
  @param [in]   type		ACL type	(ACL_TYPE_DEFAULT, ACL_TYPE_ACCESS)
  @param [in]   tag		ACL tag    (ACL_USER, ACL_USER_OBJ,.....)
  @param [out] perm	ACL permission (ACL_READ....)
  @return gboolean 성공 여부
 */
gboolean sys_acl_get_by_name(gchar *path, char *name, acl_type_t type, acl_tag_t tag, acl_perm_t *perm)
{
	id_t id = -1;

	if (tag == ACL_USER)
		id = sys_user_get_uid_from_name(name);
	else if (tag == ACL_GROUP)
		id = sys_user_get_gid_from_name(name);

	return sys_acl_get_by_id(path, id, type, tag, perm);
}

/**
  @brief   이름으로 지정된 ACL 퍼미션을 제거한다.
  @details
  		해당 entry가 없는 경우는 오류로 처리한다..
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   name	user name 혹은 group name
  @param [in]   type		ACL type	(ACL_TYPE_DEFAULT, ACL_TYPE_ACCESS)
  @param [in]   tag		ACL tag    (ACL_USER, ACL_USER_OBJ,.....)
  @return gboolean 성공 여부
 */
gboolean sys_acl_del_by_name(gchar *path, char *name, acl_type_t type, acl_tag_t tag)
{
	id_t id = -1;

	if (tag == ACL_USER)
		id = sys_user_get_uid_from_name(name);
	else if (tag == ACL_GROUP)
		id = sys_user_get_gid_from_name(name);

	return sys_acl_del_by_id(path, id, type, tag);
}

//------------------------------------------
// set (old method)
//-----------------------------------------

static gboolean _sys_acl_set(gboolean dft, int kind, gchar *path, id_t id, gchar* perm_str)
{
	acl_type_t type;
	acl_tag_t  tag = 0;
	acl_perm_t perm = 0;

	if (dft == TRUE)
		type = ACL_TYPE_DEFAULT;
	else
		type = ACL_TYPE_ACCESS;

	if (kind == 'u')
		tag = ACL_USER;
	else if (kind == 'g')
		tag = ACL_GROUP;
	else if (kind == 'm')
		tag = ACL_MASK;
	else if (kind == 'o')
		tag = ACL_OTHER;
	else
		tag = 0;

	perm = 0;
	if (perm_str[0] == 'r')
		perm |= ACL_READ;
	if (perm_str[1] == 'w')
		perm |= ACL_WRITE;
	if (perm_str[2] == 'x')
		perm |= ACL_EXECUTE;

	return sys_acl_set_by_id(path, id, type, tag, perm);

#if 0
	gchar cmd[1024];
	char	*dftStr;

	if (dft == TRUE)
		dftStr = "d:";
	else
		dftStr = "";

	if (kind == 'u' || kind == 'g')
		sprintf(cmd, "setfacl -m %s%c:%d:%s %s  2>&1", dftStr, kind, id, perm, path);
	else if (kind == 'm' || kind == 'o')
		sprintf(cmd, "setfacl -m %s%c:%s %s  2>&1", dftStr, kind, perm, path);
	else
		return FALSE;

	gboolean res;
	FILE *fp;

	fp = popen(cmd, "r");
	if (fp != NULL) {
		res = TRUE;
//	char buf[1024];
//	while (1) {
//	 	if (fgets(buf, sizeof(buf), fp) == NULL)
//			break;
//	 	//	printf("----- %s", buf);
//	 	res = FALSE;		// may be error message
//		}
		pclose(fp);
	}
	else {
		res = FALSE;
	}

	return res;
#endif
}


/**
  @brief   지정된 사용자의 ACL 퍼미션을 설정한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   user		사용자명
  @param [in]   perm		문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_set_user (gchar *path, gchar*user, gchar* perm)
{
	uid_t uid = sys_user_get_uid_from_name(user);
	return _sys_acl_set (FALSE, 'u', path, uid, perm);
}

/**
  @brief   지정된 사용자의 ACL 퍼미션을 설정한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   uid		사용자ID
  @param [in]   perm		문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_set_uid  (gchar *path, uid_t uid,  gchar* perm)
{
	return _sys_acl_set (FALSE, 'u', path, uid, perm);
}


/**
  @brief   지정된 그룹의 ACL 퍼미션을 설정한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   group	그룹명
  @param [in]   perm		문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_set_group(gchar *path, gchar*group, gchar* perm)
{
	gid_t gid = sys_user_get_gid_from_name(group);
	return _sys_acl_set(FALSE, 'g', path, gid, perm);
}

/**
  @brief   지정된 그룹의 ACL 퍼미션을 설정한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   gid 		그룹ID
  @param [in]   perm		문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_set_gid  (gchar *path, uid_t gid,   gchar* perm)
{
	return _sys_acl_set(FALSE, 'g', path, gid, perm);
}

/**
  @brief   other의 ACL 퍼미션을 설정한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   perm		문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_set_other(gchar *path, gchar* perm)
{
	return _sys_acl_set(FALSE, 'o', path, -1, perm);
}

/**
  @brief   ACL MASK를 설정한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   perm		문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_set_mask(gchar *path, gchar* perm)
{
	return _sys_acl_set(FALSE, 'm', path, -1, perm);
}

gboolean sys_acl_default_set_user (gchar *path, gchar*user, gchar* perm)
{
	uid_t uid = sys_user_get_uid_from_name(user);
	return _sys_acl_set (TRUE, 'u', path, uid, perm);
}

gboolean sys_acl_default_set_uid  (gchar *path, uid_t uid,  gchar* perm)
{
	return _sys_acl_set (TRUE, 'u', path, uid, perm);
}


gboolean sys_acl_default_set_group(gchar *path, gchar*group, gchar* perm)
{
	gid_t gid = sys_user_get_gid_from_name(group);
	return _sys_acl_set(TRUE, 'g', path, gid, perm);
}

gboolean sys_acl_default_set_gid  (gchar *path, uid_t gid,   gchar* perm)
{
	return _sys_acl_set(TRUE, 'g', path, gid, perm);
}


gboolean sys_acl_default_set_other(gchar *path, gchar* perm)
{
	return _sys_acl_set(TRUE, 'o', path, -1, perm);
}

gboolean sys_acl_default_set_mask(gchar *path, gchar* perm)
{
	return _sys_acl_set(TRUE, 'm', path, -1, perm);
}


//------------------------------------------
// delete
//-----------------------------------------

static gboolean _sys_acl_del(gboolean dft, int kind, gchar *path, id_t id)
{
	acl_type_t type;
	acl_tag_t  tag = 0;

	if (dft == TRUE)
		type = ACL_TYPE_DEFAULT;
	else
		type = ACL_TYPE_ACCESS;

	if (kind == 'u')
		tag = ACL_USER;
	else if (kind == 'g')
		tag = ACL_GROUP;
	else if (kind == 'm')
		tag = ACL_MASK;
	else if (kind == 'o')
		tag = ACL_OTHER;
	else
		tag = 0;

	return sys_acl_del_by_id(path, id, type, tag);

#if 0
	gchar cmd[1024];
	char	*dftStr;

	if (dft == TRUE)
		dftStr = "d:";
	else
		dftStr = "";

	if (kind == 'u' || kind == 'g')
		sprintf(cmd, "setfacl -x %s%c:%d %s  2>&1", dftStr, kind, id, path);
	else if (kind == 'm' || kind == 'o')
		sprintf(cmd, "setfacl -x %s%c %s  2>&1", dftStr, kind, path);
	else
		return FALSE;

	gboolean res;
	FILE *fp;

	fp = popen(cmd, "r");
	if (fp != NULL) {
		res = TRUE;
//	char buf[1024];
//	while (1) {
//	 	if (fgets(buf, sizeof(buf), fp) == NULL)
//			break;
//	 	//	printf("----- %s", buf);
//	 	res = FALSE;		// may be error message
//		}
		pclose(fp);
	}
	else {
		res = FALSE;
	}

	return res;
#endif
}

/**
  @brief   지정된 사용자의 ACL 퍼미션을 제거한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   user 		사용자명
  @return gboolean 성공 여부
 */
gboolean sys_acl_del_user (gchar *path, gchar*user)
{
	uid_t uid = sys_user_get_uid_from_name(user);
	return _sys_acl_del (FALSE, 'u', path, uid);
}

/**
  @brief   지정된 사용자의 ACL 퍼미션을 제거한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   uid 		사용자ID
  @return gboolean 성공 여부
 */
gboolean sys_acl_del_uid  (gchar *path, uid_t uid)
{
	return _sys_acl_del (FALSE, 'u', path, uid);
}

/**
  @brief   지정된 그룹의 ACL 퍼미션을 제거한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   group	그룹명
  @return gboolean 성공 여부
 */
gboolean sys_acl_del_group(gchar *path, gchar*group)
{
	gid_t gid = sys_user_get_gid_from_name(group);
	return _sys_acl_del(FALSE, 'g', path, gid);
}

/**
  @brief   지정된 그룹의 ACL 퍼미션을 제거한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   gid		그룹ID
  @return gboolean 성공 여부
 */
gboolean sys_acl_del_gid  (gchar *path, uid_t gid)
{
	return _sys_acl_del(FALSE, 'g', path, gid);
}

/**
  @brief   other의 ACL 퍼미션을 제거한다.
  @details
  			실제로는 other permission은 제거 할 수 없다 \n
  			항상 실패로 처리된다.
  @param [in]   path		파일 또는 디렉토리 경로
  @return gboolean 성공여부 (항상 FALSE)
 */
gboolean sys_acl_del_other(gchar *path)
{
	return FALSE;
	// return _sys_acl_del(FALSE, 'o', path, -1);
}

/**
  @brief   ACL MASK를 제거한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @return gboolean 성공 여부
 */
gboolean sys_acl_del_mask(gchar *path)
{
	return _sys_acl_del(FALSE, 'm', path, -1);
}

gboolean sys_acl_default_del_user (gchar *path, gchar*user)
{
	uid_t uid = sys_user_get_uid_from_name(user);
	return _sys_acl_del(TRUE, 'u', path, uid);
}
gboolean sys_acl_default_del_uid  (gchar *path, uid_t uid)
{
	return _sys_acl_del(TRUE, 'u', path, uid);
}
gboolean sys_acl_default_del_group(gchar *path, gchar*group)
{
	gid_t gid = sys_user_get_gid_from_name(group);
	return _sys_acl_del(TRUE, 'g', path, gid);
}

gboolean sys_acl_default_del_gid  (gchar *path, uid_t gid)
{
	return _sys_acl_del(TRUE, 'g', path, gid);
}

gboolean sys_acl_default_del_other(gchar *path)
{
	return _sys_acl_del(TRUE, 'o', path, -1);
}

gboolean sys_acl_default_del_mask(gchar *path)
{
	return _sys_acl_del(TRUE, 'm', path, -1);
}

//------------------------------------------
// get
//-----------------------------------------

static gboolean _sys_acl_get(gboolean dft, int kind, gchar *path, id_t id, gchar* perm_str)
{
	acl_type_t type;
	acl_tag_t  tag = 0;
	acl_perm_t  perm = 0;
	gboolean resB;

	if (dft == TRUE)
		type = ACL_TYPE_DEFAULT;
	else
		type = ACL_TYPE_ACCESS;

	if (kind == 'u')
		tag = ACL_USER;
	else if (kind == 'g')
		tag = ACL_GROUP;
	else if (kind == 'm')
		tag = ACL_MASK;
	else if (kind == 'o')
		tag = ACL_OTHER;
	else
		tag = 0;

	resB = sys_acl_get_by_id(path, id, type, tag, &perm);
	if (resB == FALSE)
		perm = 0;

	if (perm & ACL_READ)
		perm_str[0] = 'r';
	else
		perm_str[0] = '-';

  if (perm & ACL_WRITE)
		perm_str[1] = 'w';
	else
		perm_str[1] = '-';

  if (perm & ACL_EXECUTE)
		perm_str[2] = 'x';
	else
		perm_str[2] = '-';
  perm_str[3] = 0;

	return resB;
}

/**
  @brief   지정된 사용자의 ACL 퍼미션을 구한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   	user 		사용자명
  @param [out] perm	문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_get_user (gchar *path, gchar*user, gchar* perm)
{
	uid_t uid = sys_user_get_uid_from_name(user);
	return _sys_acl_get (FALSE, 'u', path, uid, perm);
}

/**
  @brief   지정된 사용자의 ACL 퍼미션을 구한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   	uid 		사용자ID
  @param [out] perm	문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_get_uid  (gchar *path, uid_t uid,  gchar* perm)
{
	return _sys_acl_get (FALSE, 'u', path, uid, perm);
}

/**
  @brief   지정된 그룹의 ACL 퍼미션을 구한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   	group	그룹명
  @param [out] perm	문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_get_group(gchar *path, gchar*group, gchar* perm)
{
	gid_t gid = sys_user_get_gid_from_name(group);
	return _sys_acl_get(FALSE, 'g', path, gid, perm);
}

/**
  @brief   지정된 그룹의 ACL 퍼미션을 구한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [in]   	gid		그룹ID
  @param [out] perm	문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_get_gid  (gchar *path, uid_t gid,   gchar* perm)
{
	return _sys_acl_get(FALSE, 'g', path, gid, perm);
}

/**
  @brief   other의 ACL 퍼미션을 구한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [out] perm	문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_get_other(gchar *path, gchar* perm)
{
	return _sys_acl_get(FALSE, 'o', path, -1, perm);
}

/**
  @brief   ACL mask를 구한다.
  @param [in]   path		파일 또는 디렉토리 경로
  @param [out] perm	문자열 형식의 퍼미션 (예) rwx
  @return gboolean 성공 여부
 */
gboolean sys_acl_get_mask(gchar *path, gchar* perm)
{
	return _sys_acl_get(FALSE, 'm', path, -1, perm);
}

gboolean sys_acl_default_get_user (gchar *path, gchar*user, gchar* perm)
{
	uid_t uid = sys_user_get_uid_from_name(user);
	return _sys_acl_get (TRUE, 'u', path, uid, perm);
}

gboolean sys_acl_default_get_uid  (gchar *path, uid_t uid,  gchar* perm)
{
	return _sys_acl_get (TRUE, 'u', path, uid, perm);
}

gboolean sys_acl_default_get_group(gchar *path, gchar*group, gchar* perm)
{
	gid_t gid = sys_user_get_gid_from_name(group);
	return _sys_acl_get(TRUE, 'g', path, gid, perm);
}

gboolean sys_acl_default_get_gid  (gchar *path, uid_t gid,   gchar* perm)
{
	return _sys_acl_get(TRUE, 'g', path, gid, perm);
}

gboolean sys_acl_default_get_other(gchar *path, gchar* perm)
{
	return _sys_acl_get(TRUE, 'o', path, -1, perm);
}

gboolean sys_acl_default_get_mask(gchar *path, gchar* perm)
{
	return _sys_acl_get(TRUE, 'm', path, -1, perm);
}

