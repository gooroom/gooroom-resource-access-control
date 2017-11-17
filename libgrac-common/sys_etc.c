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
 * sys_etc.c
 *
 *  Created on: 2016. 10. 12
 *      Author: gooroom@gooroom.kr
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
#include <limits.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/syscall.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "sys_file.h"
#include "sys_etc.h"
#include "cutility.h"
#include "grac_log.h"


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
	char	*path;

	if (name == NULL)
		return FALSE;
	*name = 0;

	path = g_file_read_link("/proc/self/exe", NULL);
	if (path != NULL) {
		char *ptr;
		ptr = c_strrchr(path, '/', PATH_MAX);
		if (ptr != NULL)
			ptr++;
		else
			ptr = path;

		if (size > c_strlen(ptr, PATH_MAX)) {
			c_strcpy(name, ptr, size);
			done = TRUE;
		}

		free(path);
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
	return sys_run_cmd_get_output(cmd, caller, NULL, 0);
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
	int		fd;
	char *buffer;
	int		buffer_size = 2048;
	char *tmp_buf;
	int		tmp_buf_size = 2048;

	if (out == NULL)
		size = 0;
	else
		*out = 0;

	buffer = c_alloc(buffer_size);
	if (buffer == NULL) {
		grac_log_error("%s() : %s : can't allocate buffer : %s", __FUNCTION__, caller, cmd);
		return FALSE;
	}
	tmp_buf = c_alloc(tmp_buf_size);
	if (tmp_buf == NULL) {
		grac_log_error("%s() : %s : can't allocate buffer : %s", __FUNCTION__, caller, cmd);
		c_free(&buffer);
		return FALSE;
	}

	fp = popen(cmd, "r");
	if (fp == NULL) {
			grac_log_debug("%s() : %s : can't run - %s", __FUNCTION__, caller, cmd);
			goto finish;
	}

	fd = fileno(fp);
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		grac_log_debug("%s() : %s : fcntl() : not set NONBLOCK: %s", __FUNCTION__, caller, c_strerror(-1));
		pclose(fp);
		goto finish;
	}

	done = TRUE;

	int data_len;
	int	res;
	int nodata = 0;

	data_len = 0;
	while (1) {
		int rest = buffer_size - data_len -1;
		if (rest > 0)
			res = sys_file_read(fd, buffer+data_len, rest);
		else
			res = sys_file_read(fd, tmp_buf, tmp_buf_size);
		if (res == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				if (nodata == 0)
					grac_log_debug("%s() : %s - no data : retry", __FUNCTION__, caller);
				nodata++;
				if (nodata > 10) {
					grac_log_debug("%s() : %s - no data : stop", __FUNCTION__, caller);
					break;
				}
				g_usleep(100*1000);
			} else {
				done = FALSE;
				grac_log_error("%s() : %s - read error", __FUNCTION__, caller, c_strerror(-1));
				break;
			}
		} else if (res == 0) {  // closed
			break;
		} else {
			nodata = 0;
			if (rest > 0)
				data_len += res;
		}
	} // while
	buffer[data_len] = 0;


	if (size > 1)
		c_strcpy(out, buffer, size);

	// log
	if (data_len > 0) {
		char *line, *ptr;
		line = buffer;
		while (1) {
			if (*line == 0)
				break;
			ptr = c_strchr(line, '\n', -1);
			if (ptr != NULL)
				*ptr = 0;
			grac_log_debug("%s : %s - %s", caller, __FUNCTION__, line);
			if (ptr == NULL)
				break;
			*ptr = '\n';
			line = ptr + 1;
		}
	}

	pclose(fp);

finish:

	c_free(&buffer);
	c_free(&tmp_buf);

	// grac_log_debug("*** end of %s : %s : %s result : %d", caller, func, cmd, (int)done);

	return done;
}


#define PIPE_READ  0
#define PIPE_WRITE 1

FILE* sys_popen(char *cmd, char *type, int *pid)
{
	pid_t child;
	int	p_fd[2];

	pipe(p_fd);

	child = fork();
	if (child == -1) {
		grac_log_error("%s(): can't fork : %s", __FUNCTION__, cmd);
		close(p_fd[PIPE_READ]);
		close(p_fd[PIPE_WRITE]);
		*pid = 0;
		return NULL;
	}

	if (child == 0) {
		if (c_strmatch(type, "r")) {
			close(p_fd[PIPE_READ]);
			dup2(p_fd[PIPE_WRITE], 1);
		} else {
			close(p_fd[PIPE_WRITE]);
			dup2(p_fd[PIPE_READ], 0);
		}

		setpgid(child, child);

		execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);

		grac_log_error("%s(): can't execl : %s", __FUNCTION__, cmd);
		exit(0);
	}


	FILE* fp;

	if (c_strmatch(type, "r")) {
		close(p_fd[PIPE_WRITE]);
		fp = fdopen(p_fd[PIPE_READ], "r");
	} else {
		close(p_fd[PIPE_READ]);
		fp = fdopen(p_fd[PIPE_WRITE], "w");
	}

	*pid = child;
	return fp;
}

gboolean sys_pclose(FILE* fp, int pid)
{
	gboolean done = FALSE;
	int	stat = -1;

	if (pid > 0) {
		kill(-pid, SIGKILL);
	}

	if (fp) {
		fclose(fp);
	}

	if (pid > 0) {
		int res = waitpid(pid, &stat, 0);
		if (res == -1) {
			if (errno == EINTR)
				done = TRUE;
		} else {
			done = TRUE;
		}
	}

	return done;
}

int sys_getpid()
{
	return getpid();
}

int sys_gettid()
{
	return syscall(__NR_gettid);
}



