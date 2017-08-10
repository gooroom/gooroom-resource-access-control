/*
 * sys_iptblables.c
 *
 *  Created on: 2016. 1. 17.
 *      Author: user
 */

/**
  @file 	 	sys_iptblables.c
  @brief		리눅스의 iptables에 의한 네트워크 사용 허가 설정 관련 함수
  @details	시스템 혹은 특정 라이브러리 의존적인 부분을 분리하는 목적의 래퍼 함수들이다.  \n
  				현재는 "iptables"  명령을 사용하였다.	\n
  				헤더파일 :  	sys_iptblables.h	\n
  				라이브러리:	libgrac.so
 */

/*
 *  Todo
 *        libiptc를 이용한 cmd 방식의 의존성 제거
 */

#include "sys_iptables.h"
#include "sys_user.h"

#include <stdio.h>

static const char *_filter_name(int filter)
{
	char *name;
	switch (filter) {
	case IPTBL_FILTER_OUTPUT :
		name = "OUTPUT";
		break;
	case IPTBL_FILTER_INPUT :
		name = "INPUT";
		break;
	case IPTBL_FILTER_FORWARD :
		name = "FORWARD";
		break;
	default :
		name = "";
		break;
	}
	return name;
}

static const char *_action_name(int action)
{
	char *name;
	switch (action) {
	case IPTBL_ACTION_ACCEPT:
		name = "ACCEPT";
		break;
	case IPTBL_ACTION_DROP:
		name = "DROP";
		break;
	default :
		name = "";
		break;
	}
	return name;
}


// 이곳에서는 결과를 무시한다
static gboolean _run_cmd (char *cmd)
{
	gboolean res;
	FILE *fp;

	fp = popen(cmd, "r");
	if (fp != NULL) {
		// char buf[1024];
		// while (1) {
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

// 하나의 rule 제거
static gboolean _cmd_del_one_rule(gboolean userB, gchar *name, int oneFilter, int action)
{
	char	cmd[256];
	const char	*filterStr;
	const char	*actionStr;
	char	*ownerStr;
	gboolean res = TRUE;

	filterStr = _filter_name(oneFilter);
	actionStr = _action_name(action);
	if (*filterStr == 0 || *actionStr == 0)
		res = FALSE;

	if (userB)
		ownerStr = "--uid-owner";
	else
		ownerStr = "--gid-owner";

	if (res == TRUE) {
		sprintf(cmd, "iptables -D %s -m owner %s %s -j %s 2>&1",
				         filterStr, ownerStr, name, actionStr);
		res = _run_cmd(cmd);
	}

	return res;
}


// 편의상 (임시)
//      1. 이 곳에서 설정하는 형식에 대해서만 삭제 처리
//      2. 기능한 조합을 반복 생성하여 삭제
static gboolean _cmd_del_all_rule(gboolean userB, gchar *name, int filter, int action)
{
	gboolean res = TRUE;

	if (filter <= 0)
		filter = IPTBL_FILTER_ALL;

	if (filter & IPTBL_FILTER_OUTPUT) {
		if (action <= 0 || action == IPTBL_ACTION_ACCEPT)
			res &= _cmd_del_one_rule(userB, name, IPTBL_FILTER_OUTPUT, IPTBL_ACTION_ACCEPT);
		if (action <= 0 || action == IPTBL_ACTION_DROP)
			res &= _cmd_del_one_rule(userB, name, IPTBL_FILTER_OUTPUT, IPTBL_ACTION_DROP);
	}

	if (filter & IPTBL_FILTER_INPUT) {
		if (action <= 0 || action == IPTBL_ACTION_ACCEPT)
			res &= _cmd_del_one_rule(userB, name, IPTBL_FILTER_INPUT, IPTBL_ACTION_ACCEPT);
		if (action <= 0 || action == IPTBL_ACTION_DROP)
			res &= _cmd_del_one_rule(userB, name, IPTBL_FILTER_INPUT, IPTBL_ACTION_DROP);
	}

	if (filter & IPTBL_FILTER_FORWARD) {
		if (action <= 0 || action == IPTBL_ACTION_ACCEPT)
			res &= _cmd_del_one_rule(userB, name, IPTBL_FILTER_FORWARD, IPTBL_ACTION_ACCEPT);
		if (action <= 0 || action == IPTBL_ACTION_DROP)
			res &= _cmd_del_one_rule(userB, name, IPTBL_FILTER_FORWARD, IPTBL_ACTION_DROP);
	}

	return res;
}

// 하나의 rule 추가
static gboolean _cmd_ins_one_rule(gboolean userB, gchar *name, int oneFilter, int action)
{
	char	cmd[256];
	const char	*filterStr;
	const char	*actionStr;
	char	*ownerStr;
	gboolean res = TRUE;

	filterStr = _filter_name(oneFilter);
	actionStr = _action_name(action);
	if (*filterStr == 0 || *actionStr == 0)
		res = FALSE;

	if (userB)
		ownerStr = "--uid-owner";
	else
		ownerStr = "--gid-owner";

	if (res == TRUE) {
		sprintf(cmd, "iptables -A %s -m owner %s %s -j %s 2>&1",
				         filterStr, ownerStr, name, actionStr);
		res = _run_cmd(cmd);
	}

	return res;
}


// 지정된 사용자에 대하여 모든 필터에 rule 추가
static gboolean _cmd_ins_all_rule(gboolean userB, gchar *name, int filter, int action)
{
	gboolean res = TRUE;

	if (filter <= 0)
		filter = IPTBL_FILTER_ALL;

	if (filter & IPTBL_FILTER_OUTPUT) {
		res &= _cmd_ins_one_rule(userB, name, IPTBL_FILTER_OUTPUT, action);
	}

	if (filter & IPTBL_FILTER_INPUT) {
		res &= _cmd_ins_one_rule(userB, name, IPTBL_FILTER_INPUT, action);
	}

	if (filter & IPTBL_FILTER_FORWARD) {
		res &= _cmd_ins_one_rule(userB, name, IPTBL_FILTER_FORWARD, action);
	}

	return res;
}

/**
  @brief   iptables에 등록되어 있는 모든 rule을 초기화한다.
  @return gboolean 성공여부
 */
gboolean sys_iptbl_init()
{
	gboolean res;
	gchar	cmd[128];

	sprintf(cmd, "iptables -F OUTPUT 2>&1");
	res = _run_cmd(cmd);
//	printf("%s %d\n", cmd, res);

//	sprintf(cmd, "iptables -P OUTPUT DROP 2>&1");
//	res &= _run_cmd(cmd);
//	printf("%s %d\n", cmd, res);

	return res;
}

/**
  @brief   iptables 관련 함수를 보두 사용한 후의 마무리 처리
 */
void sys_iptbl_clear()
{
}

/**
  @brief   iptables에 사용자에 대한 rule을 추가한다.
  @details
  			동일 사용자가 있는 경우에는 가장 마지막에 추가한 것이 유효하다.	\n
  			통신 방향	\n
  				IPTBL_FILTER_INPUT    			\n
				IPTBL_FILTER_FORWARD  		\n
				IPTBL_FILTER_OUTPUT   		\n
			허용여부	\n
				IPTBL_ACTION_ACCEPT   	\n
				IPTBL_ACTION_DROP
  @param [in] name	사용자명
  @param [in] filter	통신방향
  @param [in] action	허용여부
  @return gboolean 성공여부
 */
gboolean sys_iptbl_set_action_by_user (gchar *name, int filter, int action)
{
	gboolean res = FALSE;

	//if (action == IPTBL_ACTION_ACCEPT)
	//	res = _cmd_del_all_rule(TRUE, name, filter, IPTBL_ACTION_DROP);
	//else
	//	res = _cmd_del_all_rule(TRUE, name, filter, IPTBL_ACTION_ACCEPT);

	res = _cmd_ins_all_rule(TRUE, name, filter, action);

	return res;
}

/**
  @brief   iptables에 사용자에 대한 rule을 추가한다.
  @details
  			동일 사용자가 있는 경우에는 가장 마지막에 추가한 것이 유효하다.	\n
  			통신 방향	\n
  				IPTBL_FILTER_INPUT    			\n
				IPTBL_FILTER_FORWARD  		\n
				IPTBL_FILTER_OUTPUT   		\n
			허용여부	\n
				IPTBL_ACTION_ACCEPT   	\n
				IPTBL_ACTION_DROP
  @param [in] uid		사용자ID
  @param [in] filter	통신방향
  @param [in] action	허용여부
  @return gboolean 성공여부
 */
gboolean sys_iptbl_set_action_by_uid  (uid_t uid,   int filter, int action)
{
	gboolean res = FALSE;
	gchar	name[128];

	res = sys_user_get_name_from_uid(uid, name, sizeof(name));
	if (res == TRUE)
		res = sys_iptbl_set_action_by_user(name, filter, action);
	return res;
}


//gboolean sys_iptbl_get_action_by_user (gchar *uname, int *filter, int *action)
//gboolean sys_iptbl_get_action_by_uid  (uid_t uid,   int *filter, int *action)

/**
  @brief   지정된 사용자의 모든 rule을 삭제한다.
  @param [in] name	사용자
  @return gboolean 성공여부
 */
gboolean sys_iptbl_del_all_action_by_user (gchar *name)
{
	return _cmd_del_all_rule(TRUE, name, -1, -1);
}

/**
  @brief   지정된 사용자의 모든 rule을 삭제한다.
  @param [in] uid	사용자ID
  @return gboolean 성공여부
 */
gboolean sys_iptbl_del_all_action_by_uid  (uid_t uid)
{
	gboolean res = FALSE;
	gchar	name[128];

	res = sys_user_get_name_from_uid(uid, name, sizeof(name));
	if (res == TRUE)
		res = sys_iptbl_del_all_action_by_user(name);
	return res;
}



/**
  @brief   iptables에 그룹에 대한 rule을 추가한다.
  @details
  			동일 그룹이 있는 경우에는 가장 마지막에 추가한 것이 유효하다.	\n
  			통신 방향	\n
  				IPTBL_FILTER_INPUT    			\n
				IPTBL_FILTER_FORWARD  		\n
				IPTBL_FILTER_OUTPUT   		\n
			허용여부	\n
				IPTBL_ACTION_ACCEPT   	\n
				IPTBL_ACTION_DROP
  @param [in] name	그룹명
  @param [in] filter	통신방향
  @param [in] action	허용여부
  @return gboolean 성공여부
 */
gboolean sys_iptbl_set_action_by_group(gchar *name, int filter, int action)
{
	gboolean res = TRUE;

	//if (action == IPTBL_ACTION_ACCEPT)
	//	res = _cmd_del_all_rule(FALSE, name, filter, IPTBL_ACTION_DROP);
	//else
	//	res = _cmd_del_all_rule(FALSE, name, filter, IPTBL_ACTION_ACCEPT);

	res &= _cmd_ins_all_rule(FALSE, name, filter, action);

	return res;
}

/**
  @brief   iptables에 그룹에 대한 rule을 추가한다.
  @details
  			동일 그룹이 있는 경우에는 가장 마지막에 추가한 것이 유효하다.	\n
  			통신 방향	\n
  				IPTBL_FILTER_INPUT    			\n
				IPTBL_FILTER_FORWARD  		\n
				IPTBL_FILTER_OUTPUT   		\n
			허용여부	\n
				IPTBL_ACTION_ACCEPT   	\n
				IPTBL_ACTION_DROP
  @param [in] gid		그룹ID
  @param [in] filter	통신방향
  @param [in] action	허용여부
  @return gboolean 성공여부
 */
gboolean sys_iptbl_set_action_by_gid  (gid_t gid,    int filter, int action)
{
	gboolean res = FALSE;
	gchar	name[128];

	res = sys_user_get_name_from_gid(gid, name, sizeof(name));
	if (res == TRUE)
		res = sys_iptbl_set_action_by_group(name, filter, action);
	return res;

}

//gboolean sys_iptbl_get_action_by_group(gchar *gname, int *filter, int *action);
//gboolean sys_iptbl_get_action_by_gid  (gid_t gid,    int *filter, int *action);

/**
  @brief   지정된 그룹의 모든 rule을 삭제한다.
  @param [in] name	그룹명
  @return gboolean 성공여부
 */
gboolean sys_iptbl_del_all_action_by_group(gchar *name)
{
	return _cmd_del_all_rule(FALSE, name, -1, -1);
}

/**
  @brief   지정된 그룹의 모든 rule을 삭제한다.
  @param [in] gid	그룹ID
  @return gboolean 성공여부
 */
gboolean sys_iptbl_del_all_action_by_gid  (gid_t gid)
{
	gboolean res = FALSE;
	gchar	name[128];

	res = sys_user_get_name_from_gid(gid, name, sizeof(name));
	if (res == TRUE)
		res = sys_iptbl_del_all_action_by_group(name);
	return res;

}
