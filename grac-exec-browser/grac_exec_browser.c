/*
 ============================================================================
 Name        :
 Author      : 
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */

/**
 @mainpage grac-exec
 @section intro 소개
   - grac-agent가 요청한 프로그램(command)을 실행시킨다.
                사용법   grac-exec  path  {argument}
   - grac-agent는 루트 사용자로 실행되는 반면 grac-exec는 실행하고자하는 프로그램과 동일한 사용자 (grsu_xx)로 실행하게 된다.
   - grac-exec는 grac-agent에 의해 실행되며 사용자가 직접 사용할 경우는 없다.
      -  만약 사용자가 명령창을 통해 직접 실행하는 경우 해당 프로그램을 직접 실행한 것과 같다.
 @section Program 프로그램명
   - grac-exec
 @section CREATEINFO 작성정보
      - 작성일 : 2017.04.05
 **/

/**
  @file  grac_exec.c
  @brief main flow of grac_exec
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include "grm_log.h"
#include "grac_browser.h"
#include "cutility.h"

typedef struct _CmdLine {

	char	url[2048];

	char*  cmd;
	int		 argc;
	char** argv;
} CmdLine;


#if 0
extern char **environ;

#define MAX_ENV_CNT 100
char *g_new_env[MAX_ENV_CNT] = { 0 };   // 현재 환경변수 + IME용 환경 변수 보관
static void free_env();

/**
    요청된 프로그램을 실핼하기 위한 환경변수를 구하여 g_new_env[]에 보관
    1. 현재 설정되어 있는 환경 정보를 구한다.
    2. 한글 IME가 동작하기 위한 환경변수를 추가한다.
 */
static gboolean init_env()
{
	int idx = 0;
	char *ptr;

	memset(g_new_env, 0, sizeof(g_new_env));

	for (idx=0; idx<MAX_ENV_CNT; idx++) {
		ptr = environ[idx];
		if (ptr == NULL)
			break;
		g_new_env[idx] = strdup(ptr);
	}

	if (idx + 5 > MAX_ENV_CNT) {
		grm_log_debug("init_env() : out of memory");
		free_env();
		return FALSE;
	}

	// 한글 IME용 환경 변수
	g_new_env[idx++] = strdup("XMODIFIERS=@im=uim");
	g_new_env[idx++] = strdup("QT_IM_MODULE=xim");
	g_new_env[idx++] = strdup("QT4_IM_MODULE=xim");
	g_new_env[idx++] = strdup("GTK_IM_MODULE=uim");
	g_new_env[idx++] = strdup("CLUTTER_IM_MODULE=xim");

	return TRUE;

}

// 환경변수 보관에 사용된 버퍼 해제
static void free_env()
{
	int idx;
	for (idx=0; idx<MAX_ENV_CNT; idx++) {
		if (g_new_env[idx] == NULL)
			break;
		free(g_new_env[idx]);
		g_new_env[idx] = NULL;
	}
}
#endif


void display_help()
{
	printf("grac-exec-browser [option] [cmd [arg...]]\n");
	printf("option\n");
	printf("  -u URL     -> URL to display \n");
	printf("  -h         -> display help\n");
}

int check_command(int argc, char *argv[], CmdLine *cmd)
{
	memset(cmd, 0, sizeof(CmdLine));

	int idx;

	for (idx=1; idx<argc; idx++) {
		char *ptr;
		ptr = argv[idx];
		if (*ptr == '-') {
				int opt;
				ptr++;
				opt = *ptr++;
				if (*ptr != 0)
					return 0;
				switch (opt) {
					case 'h':
						return 0;
					case 'u':
						if (idx >= argc-1)
							return 0;
						c_strcpy(cmd->url, argv[idx+1], sizeof(cmd->url));
						idx++;
						break;
					default :
						return 0;
				}
		}
		else {
			break;
		}
	}

	if (idx < argc) {
		cmd->cmd =  argv[idx];
		idx++;
		if (idx < argc) {
			cmd->argc = argc - idx;
			cmd->argv = argv + idx;
		}
	}

	return 1;
}

/**
@brief main function
@details
     현재 설정되어 있는 환경변수와  한글IME에 필요한 한경변수를 추가 설정후 요청 프로그램을 실행한다.
@return integer 0:성공  -1: 실패
 */

int main(int argc, char *argv[])
{
	CmdLine	cmd;

	if (check_command(argc, argv, &cmd) == 0) {
		display_help();
		return -1;
	}

/*
	printf("URL : %s\n", cmd.url);
	if (cmd.cmd) {
		printf("cmd : %s\n", cmd.cmd);
		printf("argc : %d\n", cmd.argc);
		for (i=0; i<cmd.argc; i++)
			printf("argv[%d] : %s\n", i, cmd.argv[i]);
	}
*/

	GracBrowser *ctrl = grac_browser_alloc();

	if (ctrl == NULL) {
		printf("Can't alloc grac_browser\n");
		return -1;
	}

	if (cmd.cmd)
		grac_browser_executeV(ctrl, cmd.url, cmd.cmd, cmd.argc, cmd.argv);
	else
		grac_browser_exec(ctrl, cmd.url);

	grac_browser_free(&ctrl);

	return 0;
}


