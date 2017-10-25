/*
 * sys_cups.c
 *
 *  Created on: 2016. 1. 18.
 *      Author: user
 */

/**
  @file 	 	sys_cups.c
  @brief		리눅스의 cups server 설정 관련 함수
  @details	시스템 혹은 특정 라이브러리 의존적인 부분을 분리하는 목적의 래퍼 함수들이다.  \n
  				현재는 libcups를을 이용하여 구현되었다.	\n
  				헤더파일 :  	sys_ cups.h	\n
  				라이브러리:	libgrac.so
 */



#include "sys_cups.h"

#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <cups/cups.h>

#include "grm_log.h"

// 2016.5  update by using cups API

static GPtrArray *g_allow_user = NULL;
static GPtrArray *g_deny_user = NULL;
//static GPtrArray *g_printer_list = NULL;

static cups_dest_t *g_printer_dests = NULL;
static int g_printer_count = 0;

static gboolean _init_buf()
{
	if (g_allow_user == NULL) {
		g_allow_user = g_ptr_array_new();
		if (g_allow_user == NULL) {
			return FALSE;
		}
	}

	if (g_deny_user == NULL) {
		g_deny_user = g_ptr_array_new();
		if (g_deny_user == NULL) {
			g_ptr_array_free(g_allow_user, TRUE);
			g_allow_user = NULL;
			return FALSE;
		}
	}

	return TRUE;
}

static void _free_buf()
{
	if (g_allow_user != NULL) {
		g_ptr_array_free(g_allow_user, TRUE);
		g_allow_user = NULL;
	}

	if (g_deny_user != NULL) {
		g_ptr_array_free(g_deny_user, TRUE);
		g_deny_user = NULL;
	}

#if _OLD
	if (g_printer_list != NULL) {
		g_ptr_array_free(g_printer_list, TRUE);
		g_printer_list = NULL;
	}
#endif
}

#if _OLD  // old method
static void _get_word(char *buf, char *word)
{
	while (isspace(*buf & 0x0ff) ) buf++;

	while (! isspace(*buf & 0x0ff) )
		*word ++ = *buf++;
	*word = 0;
}

gboolean _MakePrinterList(GPtrArray *list)
{
	void	printer_list()
	{
		int	i, num_dests;
		cups_dest_t *dests;
		cups_dest_t *default_dest;

		// num_dests = cupsGetDests2(CUPS_HTTP_DEFAULT, &dests); same results
		num_dests = cupsGetDests(&dests);
		printf("Printer Count : %d\n", num_dests);
		if (num_dests == 0) {
			printf("There is no printer\n");
			return;
		}
		for (i=0; i<num_dests; i++) {
			printf("%d: %s\n", i, dests[i].name);
		}

		default_dest = cupsGetDest(NULL, NULL, num_dests, dests);
		if (default_dest) {
			printf("default printer : %s\n", default_dest->name);
		}
		else {
			printf("Can't get default printer\n");
		}

		cupsFreeDests(num_dests, dests);


	gboolean res = TRUE;
	FILE *fp;

	fp = popen("lpstat -a 2>&1", "r");
	if (fp != NULL) {
		char buf[512], word[512];
		while (1) {
			if (fgets(buf, sizeof(buf), fp) == NULL)
				break;
			_get_word(buf, word);

			if (!strcmp(word, "lpstat:")) {	// No printer
				break;
			}
			if (strlen(word) > 0) {		// need more detailed checking !!!!!
				char *ptr = malloc(strlen(word)+1);
				if (ptr == NULL) {
					res = FALSE;
					break;
				}
				else {
					strcpy(ptr, word);
					g_ptr_array_add(list, ptr);
				}
			}
		}
		pclose(fp);
	}
	else {
		res = FALSE;
	}

	// Test Code
	//if (res) {
	//	int i;
	//	for (i=0; i<g_printer_list->len; i++)
	//		printf("%d : %s\n", i, g_ptr_array_index(g_printer_list, i));
	//}

	return res;
}
#endif

// 프린터 목록을 구한다.
static gboolean _InitPrinterList()
{
	if (g_printer_dests == NULL) {
		// g_printer_count = cupsGetDests2(CUPS_HTTP_DEFAULT, &g_printer_dests); same results
		g_printer_count = cupsGetDests(&g_printer_dests);
		if (g_printer_count == 0) {
				grm_log_debug("Debug: cupsGetDests(): not found any  printer in system");
		}
	}

	return TRUE;
}

static void _FreePrinterList()
{
	if (g_printer_dests != NULL) {
			cupsFreeDests(g_printer_count, g_printer_dests);

			g_printer_dests = NULL;
			g_printer_count = 0;
	}
}

/**
  @brief   cups 설정을 위한 준비 작업
  @details
  		내부 버퍼 확보, 시스템에 설치된 프린터 목록 생성
  @return gboolean 성공 여부
 */
gboolean sys_cups_access_init()
{
	gboolean res = TRUE;

	res = _init_buf();

	res &= _InitPrinterList();

#if _OLD
	if (g_printer_list == NULL) {
		g_printer_list = g_ptr_array_new();
		if (g_printer_list == NULL) {
 			res = FALSE;
		}
		else {
			res = _MakePrinterList(g_printer_list);
		}
	}
#endif

	if (res == FALSE)
		_free_buf();

	return res;
}

/**
  @brief   cups 설정에 사용된 버퍼 해제등 마무리 작업
 */
void sys_cups_access_final()
{
	_FreePrinterList();

	_free_buf();
}

/**
  @brief   시스템에 설정된 프린터 갯수를 구한다.
  @return int 프린터 갯수
 */
int			 sys_cups_printer_count()
{
	_FreePrinterList();   // to reload the printer-list
	_InitPrinterList();

	return g_printer_count;
}

/**
  @brief   지정된 위치의 프린터 이름을 구한다.
  @param	idx	프린터 목록에서의 위치 (0부터 갯수-1)
  @return char* 프린터 이름
 */
const char* sys_cups_get_printer_name(int idx)
{
	// not reload
	_InitPrinterList();

	if (idx >= 0 && idx < g_printer_count) {
		return g_printer_dests[idx].name;
	}

	return NULL;
}

static int _cups_is_class_type(http_t *http, char *printer)
{
	ipp_t			*request,	*response;
	ipp_attribute_t	*attr;
	cups_ptype_t		type;
	char	printer_uri[HTTP_MAX_URI];
	const char	*server = cupsServer();		// "localhost"
	int		res;

	httpAssembleURIf(HTTP_URI_CODING_ALL, printer_uri, sizeof(printer_uri),
					  	  	  "ipp", NULL, server, ippPort(),
							  	  "/printers/%s", printer);

	request = ippNewRequest(IPP_GET_PRINTER_ATTRIBUTES);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
			   	   	   "printer-uri", NULL, printer_uri);
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
			   	   	   "requested-attributes", NULL, "printer-type");
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
			   	   	   "requesting-user-name", NULL, cupsUser());

	response = cupsDoRequest(http, request, "/");

	attr = ippFindAttribute(response, "printer-type", IPP_TAG_ENUM);
	if (attr != NULL) {
    	type = (cups_ptype_t)ippGetInteger(attr, 0);
	}
	else {
		type = CUPS_PRINTER_LOCAL;
	}

  ippDelete(response);

  if (type & (CUPS_PRINTER_CLASS | CUPS_PRINTER_IMPLICIT))
	  res = 1;
  else
	  res = 0;

  return res;
}

// 허용 혹은 거부된 사용자,그룹 목록을 일괄적으로 cups에 반영
static gboolean _cups_apply_acees_printer(char *printer, gboolean allow)
{
	gboolean resB = TRUE;
	http_t	*http;
	char	*attr_users;
	char	*attr_name, *default_user;
  int		num_options;
  cups_option_t	*options;
  int i, max_len, user_cnt;
	GPtrArray *user_list;

	default_user = NULL;
	if (allow == TRUE) {
		user_list = g_allow_user;
		if (user_list == NULL || user_list->len <= 0) {	// 목록이 없는 경우,  allow all users
			default_user = "all";
			attr_name = "requesting-user-name-allowed";		// allow:all 로  대치 (모두 허용)
		}
		else {
			default_user = "root";		// add default user "root"
			attr_name = "requesting-user-name-allowed";
		}
	}
	else {
		user_list = g_deny_user;
		if (user_list == NULL || user_list->len <= 0) {	// 목록이 없는 경우, only allow root
		  default_user = "root";
		  attr_name = "requesting-user-name-allowed";
		}
		else {
			attr_name = "requesting-user-name-denied";
		}
	}

	// get length of user list
	if (user_list == NULL || user_list->len <= 0)
		user_cnt = 0;
	else
		user_cnt = user_list->len;

	max_len = 0;
	if (default_user)
		max_len = strlen(default_user);
	for (i=0; i<user_cnt; i++) {
		char *ptr = (char*)g_ptr_array_index(user_list, i);
		max_len += strlen(ptr)+1;
	}
	max_len++;

	attr_users = malloc(max_len);
	if (attr_users == NULL) {
		grm_log_error("Error: cups_apply_acees_printer: out of memory");
		return FALSE;
	}

	http        = NULL;
	num_options = 0;
	options     = NULL;

	// 목록을 하나의 문자열로 만든다.
	*attr_users = 0;
	if (default_user)
		strcpy(attr_users, default_user);
	for (i=0; i<user_cnt; i++) {
		char *ptr = (char*)g_ptr_array_index(user_list, i);
		if (attr_users[0] != 0)
			strcat(attr_users, ",");
		strcat(attr_users, ptr);
	}

  num_options = cupsAddOption(attr_name, attr_users, num_options, &options);

	http = httpConnectEncrypt(cupsServer(), ippPort(), cupsEncryption());
	if (http == NULL) {
			grm_log_error("Error: cups_apply_acees_printer: httpConnectEncrypt() - %s", strerror(errno));
			resB = FALSE;
	}
	else {
		ipp_t		*request;
		char		uri[HTTP_MAX_URI];

		if (_cups_is_class_type(http, printer)) {
			httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri),
	    										"ipp", NULL, cupsServer(), ippPort(),
													"/classes/%s", printer);
			request = ippNewRequest(CUPS_ADD_MODIFY_CLASS);
		}
		else {
			httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri),
							  	  	  "ipp", NULL, cupsServer(), ippPort(),
									  	  "/printers/%s", printer);
			request = ippNewRequest(CUPS_ADD_MODIFY_PRINTER);
		}

		ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,  "printer-uri", NULL, uri);
		ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsUser());

		cupsEncodeOptions2(request, num_options, options, IPP_TAG_PRINTER);

		ippDelete(cupsDoRequest(http, request, "/admin/"));

		if (cupsLastError() > IPP_OK_CONFLICT) {
			grm_log_error("Error: cups_apply_acees_printer: %s", cupsLastErrorString());
			resB = FALSE;
		}

	  if (http)
		  httpClose(http);
	}

	free(attr_users);

	return resB;

#if _OLD
	static char	cmd[2048];
	GPtrArray *list;
	char	*option;

	if (access == CUPS_ACCESS_ALLOW) {
		list = g_allow_user;
		if (list->len <= 0)
			option = "-u allow:root";
		else
			option = "-u allow:root,";	// notice "," at the end of string
	}
	else if (access == CUPS_ACCESS_DENY) {
		list = g_deny_user;
		if (list->len <= 0)
			option = "-u allow:all";
		else
			option = "-u deny:";
	}
	else {
		return FALSE;
	}
	sprintf(cmd, "lpadmin -p %s %s", printer, option);

	int i, cnt;
	if (list == NULL)
		cnt = 0;
	else
		cnt = list->len;
	for (i=0; i<cnt; i++) {
		char *user = (char*)g_ptr_array_index(list, i);
		if (strlen(cmd)+strlen(user)+1 > sizeof(cmd))   // too big length
			return FALSE;
		if (i > 0)
			strcat(cmd, ",");
		strcat(cmd, user);
	}
	strcat(cmd, " 2>&1");

	FILE *fp;

	fp = popen(cmd, "r");
	if (fp != NULL) {
		res = TRUE;
		// need more detailed checking !!!!!
		pclose(fp);
	}
	return res;

#endif

}

/**
  @brief   전체 프린터에 대하여 사용자,그룹 목록을 실질적으로 cups 시스템에 반영한다.
  @details
  		cups 설정은 한 번에 해야하기 때문에 미리 목록을 만들어 놓아야한다. \n
		목록은 sys_cups_access_add_user ()와 sys_cups_access_add_group()을 이용한다.
  @param	[in]	allow 프린터 허용 여부 (TRUE 혹은 FALSE)
  @return gboolean 성공여부
 */
gboolean sys_cups_access_apply(gboolean allow)
{
	gboolean resB = TRUE;
	int i, count;
	const char* printer;

	count = sys_cups_printer_count();
	for (i=0; i<count; i++) {
		printer = sys_cups_get_printer_name(i);
		if (printer) {
			resB &= _cups_apply_acees_printer((char*)printer, allow);
		}
	}

#if _OLD
	if (g_printer_list->len <= 0)
		return FALSE;

	resB = TRUE;
	for (i=0; i<g_printer_list->len; i++) {
		char *printer = (char*)g_ptr_array_index(g_printer_list, i);
		resB &= _cups_apply_acees_printer(printer, access);
	}
#endif

	return resB;
}

/**
  @brief   지정한 프린터에  대하여 사용자,그룹 목록을 실질적으로 cups 시스템에 반영한다.
  @details
  		cups 설정은 한 번에 해야하기 때문에 미리 목록을 만들어 놓아야한다. \n
		목록은 sys_cups_access_add_user ()와 sys_cups_access_add_group()을 이용한다.
  @param	[in]	idx		적용할 프린터
  @param	[in]	allow 프린터 허용 여부 (TRUE 혹은 FALSE)
  @return gboolean 성공여부
 */
gboolean sys_cups_access_apply_by_index(int idx, gboolean allow)
{
	gboolean resB = TRUE;
	const char* printer;

	printer = sys_cups_get_printer_name(idx);
	if (printer) {
		resB = _cups_apply_acees_printer((char*)printer, allow);
	}

	return resB;
}

/**
  @brief   지정한 프린터에  대하여 사용자,그룹 목록을 실질적으로 cups 시스템에 반영한다.
  @details
  		cups 설정은 한 번에 해야하기 때문에 미리 목록을 만들어 놓아야한다. \n
		목록은 sys_cups_access_add_user ()와 sys_cups_access_add_group()을 이용한다.
  @param	[in]	name	적용할 프린터 이름
  @param	[in]	allow 프린터 허용 여부 (TRUE 혹은 FALSE)
  @return gboolean 성공여부
 */
gboolean sys_cups_access_apply_by_name(char *name, gboolean allow)
{
	gboolean resB = FALSE;
	int i, count;
	const char* printer;

	if (name == NULL) {
		grm_log_debug("Debug: sys_cups_access_apply_by_name(): invalid printer (NULL)");
		return FALSE;
	}

	count = sys_cups_printer_count();
	for (i=0; i<count; i++) {
		printer = sys_cups_get_printer_name(i);
		if (printer && !strcmp(printer, name)) {
			resB = _cups_apply_acees_printer((char*)printer, allow);
			return resB;
		}
	}

	grm_log_debug("Debug: sys_cups_access_apply_by_name(): invalid printer (%s)", name);
	return FALSE;
}


/**
  @brief   설정 대상 목록에 사용자를 추가한다.
  @details
  		cups 설정은 한 번에 해야하기 때문에 미리 목록을 만들어 놓아야한다.
  @param	[in]	user		사용자 이름
  @param	[in]	allow 프린터 허용 여부 (TRUE 혹은 FALSE)
  @return gboolean 성공여부
 */
gboolean sys_cups_access_add_user (gchar *user, gboolean allow)
{
	if (_init_buf() == FALSE) {
		grm_log_error("Error: sys_cups_access_add_user: out of memory");
		return FALSE;
	}

	char *ptr = malloc(strlen(user)+1);
	if (ptr == NULL) {
		grm_log_error("Error: sys_cups_access_add_user: out of memory");
		return FALSE;
	}

	strcpy(ptr, user);
	if (allow == TRUE)
		g_ptr_array_add(g_allow_user, ptr);
	else
		g_ptr_array_add(g_deny_user, ptr);

	return TRUE;
}

/**
  @brief   설정 대상 목록에 그룹을 추가한다.
  @details
  		cups 설정은 한 번에 해야하기 때문에 미리 목록을 만들어 놓아야한다.
  @param	[in]	group	그룹 이름
  @param	[in]	allow 프린터 허용 여부 (TRUE 혹은 FALSE)
  @return gboolean 성공여부
 */
gboolean sys_cups_access_add_group(gchar *group, gboolean allow)
{
	gboolean resB;
	char	cupsGroup[256];

	if (group[0] != '@') {
		cupsGroup[0] = '@';
		strncpy(&cupsGroup[1], group, sizeof(cupsGroup)-1);
		resB = sys_cups_access_add_user (cupsGroup, allow);
	}
	else {
		resB = sys_cups_access_add_user (group, allow);
	}

	return resB;
}

