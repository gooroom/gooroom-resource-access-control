/*
 * sys_etc.c
 *
 *  Created on: 2016. 10. 12.
 *      Author: user
 */

/**
  @file 	 	sys_etc.c
  @brief		리눅스 시스템 관련 여러 함수들
  @details	헤더파일 :  	sys_etc.h	\n
  				라이브러리:	libgrac.so
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <string.h>

#include "sys_etc.h"
#include "grm_log.h"
#include "cutility.h"


/**
  @brief   이 함수를 호출한 프로세스의 이름을 구한다.
  @details
  		프로세스명에 디렉토리는 포함되지 않음에 주의한다.
  @param [out]  name	프로세스명
  @param [in] 	 size		버퍼 크기
  @return gboolean 성공 여부
 */
gboolean sys_get_current_process_name(gchar *name, int size)
{
	gboolean done = FALSE;
	char	module[PATH_MAX+1];
	int		res;

	if (name == NULL)
		return FALSE;
	*name = 0;

	c_memset(module, 0, sizeof(module));
	res = readlink("/proc/self/exe", module, sizeof(module)-1);
	if (res > 0) {
		char *ptr;
		ptr = c_strrchr(module, '/', sizeof(module));
		if (ptr != NULL)
			c_strcpy(module, ptr+1, sizeof(module));

		if (size > c_strlen(module, sizeof(module))) {
			c_strcpy(name, module, size);
			done = TRUE;
		}
	}

	return done;
}

/**
  @brief   command를 실행시킨다.
  @details
  		command와 파라메터 모두를 하나의 문자열로 구성하여 호출한다.		\n
  		pipe를 연결하여 실행시키고 모든 출력은 무시한다.							\n
  		 	실행 성공 여부는 exit code로 판별한다.
  @param [in]  cmd			실행시킬 command
  @param [in]  caller 	로그를 구분하는 목적으로사용
  @return gboolean 성공 여부
 */
gboolean sys_run_cmd_no_output(gchar *cmd, char *caller)
{
	gboolean res = FALSE;
	FILE *fp;

	fp = popen(cmd, "r");
	if (fp != NULL) {
//	char buf[1024];
//		grm_log_debug("* run_cmd() in a %s - [%s]", caller, cmd);
//	while (1) {
// 		if (fgets(buf, sizeof(buf), fp) == NULL)
//				break;
//		grm_log_debug("* run_cmd() in a %s - %s", caller, buf);
//		}
//	grm_log_debug("* run_cmd() in a %s ------- (end)", caller);

		int status = pclose(fp);
		int exit_code = -1;

		if (WIFEXITED(status)) {
			exit_code = WEXITSTATUS(status);
			if (exit_code == 0) {
				res = TRUE;
			}
			else if (exit_code >= 128) {
				int sig_num = exit_code - 128;
				if (sig_num == SIGPIPE)
					res = TRUE;
			}
		}

		if (res == FALSE)
			grm_log_error("%s : sys_run_cmd_no_output() : %s -> %d", caller, cmd, status);
	}
	else {
		grm_log_error("%s : sys_run_cmd_no_output() : %s : can't create pipe", caller, cmd);
	}

	return res;
}

/**
  @brief   command를 실행시켜 command의 출력은 지정한 버퍼에 저장한다.
  @details
  		command와 파라메터 모두를 하나의 문자열로 구성하여 호출한다.		\n
  		pipe를 연결하여 실행시키고 모든 출력은 로그로 저장하게 구현되었다.	\n
  		무한 대기 현상을 막기 위해서 non-blocking mode로 pipe를 검사하기 때문에 \n
  		일정 시간 데이터가 없으면 종료된 것으로 간주한다.
  @param [in]  cmd	실행시킬 command
  @param [in]  caller	로그를 구분하는 목적으로사용
  @param [out] out	출력을 담을 버퍼
  @param [in]   size	버퍼 크기
  @return gboolean 성공 여부
 */
gboolean sys_run_cmd_get_output(gchar *cmd, char *caller, char *out, int size)
{
	gboolean done = FALSE;
	FILE *fp;
	char *func = "sys_run_cmd_get_output()";

	fp = popen(cmd, "r");
	if (fp == NULL) {
			grm_log_debug("%s : %s : can't run - %s", caller, func, cmd);
			return FALSE;
	}

	int fd = fileno(fp);
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		grm_log_debug("%s : %s : fcntl() : not set NONBLOCK: %s", caller, func, strerror(errno));
		pclose(fp);
		return FALSE;
	}

	done = TRUE;

	char buf[1024];
	int idx = 0, cnt, res;
	int nodata = 0;
	int	outcnt = 0;

	*out = 0;
	while (1) {
		cnt = sizeof(buf) - idx;
		res = read(fd, &buf[idx], cnt-1);
		if (res == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				if (nodata == 0)
					grm_log_debug("%s : %s () - no data : retry", caller, func);
				nodata++;
				if (nodata > 3) {
					grm_log_debug("%s : %s () - no data : stop", caller, func);
					break;
				}
				usleep(100*1000);
			}
			else {
				break;
			}
		}
		else if (res == 0) {  // closed
			break;
		}
		else {
			nodata = 0;
			if (outcnt < size -1) {
				int rest = size - outcnt - 1;
				if (res < rest)
					rest = res;
				c_memcpy(out+outcnt, buf, rest);
				outcnt += rest;
				out[outcnt] = 0;
			}

			gboolean stop = FALSE;
			int i;
			idx += res;
			buf[idx] = 0;

			while (!stop) {  // 라인 단위로 로그에 저장
					stop = TRUE;
					for (i=0; i<idx; i++) {
							if (buf[i] == 0x0d || buf[i] == 0x0a) {
								stop = FALSE;
								buf[i] = 0;
								grm_log_debug("%s : %s - %s", caller, func, buf);
								if (buf[i] == 0x0d || buf[i+1] == 0x0a)
									i++;
								i++;
								c_memcpy(buf, &buf[i], idx-i);
								idx -= i;
								buf[idx] = 0;
								break;
							}
					}
			}
		}
	} // while

	if (idx)
			grm_log_debug("%s : %s - %s", caller, func, buf);
	if (res == -1)
		grm_log_debug("%s : %s - error : %s", caller, func, strerror(errno));

	pclose(fp);

	grm_log_debug("*** end of %s : %s : %s result : %d", caller, func, cmd, (int)done);
	return TRUE;
}

/**
  @brief   지정된 사용자에 의해 실행중인 프로세스가 있는지 확인한다.
  @details
  		리눅스 명령어 "ps"를 이용하여 구현되었다.
  @param [in]  username	사용자명
  @param [in]  exec			프로세스명
  @return gboolean 성공 여부
 */
int sys_check_running_process(char *username, char *exec)
{
	int		res;
	char	cmd[256];
	char  outbuf[2048];

	if (c_strlen(username, 256) == 0)
		return -1;

	snprintf(cmd, sizeof(cmd), "ps --noheader -f -u %s", username);

	gboolean done = sys_run_cmd_get_output(cmd, "check_running", outbuf, sizeof (outbuf));
	if (done) {
		int  n1, n2;
		char *ptr1, *ptr2;
		n2 = c_strlen(username, 256);
		res = 0;
		ptr1 = outbuf;
		while (1) {
			n1 = c_strlen(ptr1, sizeof(outbuf));
			if (n1 <= n2)
				break;
			if (c_memcmp(ptr1, username, n2, -1) == 0) {
				ptr2 = c_strstr(ptr1+n2, exec, sizeof(outbuf)-n2);
				// 찾아진 결과가 긴이름을 가진 프로세스명의 일부인지를 확인
				if (ptr2 && ptr1[n2] > 0 && ptr1[n2] <= 0x20) {
					res = 1;
					break;
				}
			}
			ptr2 = c_strchr(ptr1, '\n', sizeof(outbuf));
			if (ptr2 == NULL)
				break;
			ptr1 = ptr2 + 1;
		}
	}
	else {
		res = -1;
	}

	return res;
}

