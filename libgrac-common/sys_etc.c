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
#include <limits.h>
#include <sys/wait.h>
#include <signal.h>

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
				if (nodata > 10) {
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

//	grm_log_debug("*** end of %s : %s : %s result : %d", caller, func, cmd, (int)done);
	return done;
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


#define PIPE_READ  0
#define PIPE_WRITE 1

FILE* sys_popen(char *cmd, char *type, int *pid)
{
	pid_t child;
	int	p_fd[2];

	pipe(p_fd);

	child = fork();
	if (child == -1) {
		grm_log_error("%s(): can't fork : %s", __FUNCTION__, cmd);
		close(p_fd[PIPE_READ]);
		close(p_fd[PIPE_WRITE]);
		*pid = 0;
		return NULL;
	}

	if (child == 0) {
		if (c_strmatch(type, "r")) {
			close(p_fd[PIPE_READ]);
			dup2(p_fd[PIPE_WRITE], 1);
		}
		else {
			close(p_fd[PIPE_WRITE]);
			dup2(p_fd[PIPE_READ], 0);
		}

		setpgid(child, child);

		execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);

		grm_log_error("%s(): can't execl : %s", __FUNCTION__, cmd);
		exit(0);
	}


	FILE* fp;

	if (c_strmatch(type, "r")) {
		close(p_fd[PIPE_WRITE]);
		fp = fdopen(p_fd[PIPE_READ], "r");
	}
	else {
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
	int	res;

	if (pid > 0) {
		kill (-pid, SIGKILL);
	}

	if (fp) {
		fclose(fp);
	}

	if (pid > 0) {
		res = waitpid(pid, &stat, 0);
		if (res == -1) {
			if (errno == EINTR)
				done = TRUE;
		}
		else {
			done = TRUE;
		}
	}

	return done;
}


//G_LOCK_DEFINE_STATIC (run_cmd_lock);

typedef struct _RunCmdCtrl {
	gboolean stop;
	gboolean on_thread;
	pid_t	pid;
	FILE	*fp;
	int		fd;
	char	*buf;
	int		buf_size;
	int		buf_len;
	char	*cmd;
	char	*caller;
	char	tmp_buf[1024];
	int		tmp_len;
	int		tmp_size;
} RunCmdCtrl;

static void* run_cmd_thread(void *data)
{
	RunCmdCtrl *ctrl = (RunCmdCtrl*)data;
	int	fd;
	pid_t	pid;
	int	rest, buf_size;

	grm_log_debug("%s() : %s : start run_cmd_thread : pid = %d",  __FUNCTION__, ctrl->caller, getpid());

	ctrl->on_thread = TRUE;
	ctrl->stop = FALSE;

	if (ctrl->buf) {
		c_memset(ctrl->buf, 0, ctrl->buf_size);
		buf_size = ctrl->buf_size;
	}
	else {
		buf_size = 0;
	}

	ctrl->buf_len = 0;

	c_memset(ctrl->tmp_buf, 0, sizeof(ctrl->tmp_buf));
	ctrl->tmp_size = sizeof(ctrl->tmp_buf)-1;
	ctrl->tmp_len = 0;

	fd  = ctrl->fd;
	pid = ctrl->pid;

	while (ctrl->stop == FALSE) {
			int n, read_len;
			n = ctrl->tmp_size - ctrl->tmp_len;
			if (n > 0) {
				read_len = read(fd, &ctrl->tmp_buf[ctrl->tmp_len], n);
				if (read_len == -1) {
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						grm_log_debug("%s() : %s - no data : retry", __FUNCTION__, ctrl->caller);
						usleep(10*1000);
						continue;
					}
					else {
						grm_log_error("%s() : %s - read error : %s", __FUNCTION__, ctrl->caller, strerror(errno));
						break;
					}
				}
				else if (read_len == 0) {  // closed
					break;
				}
			}

			rest = buf_size - ctrl->buf_len - 1;
			if (rest > 0) {
				int copy_len = read_len;
				if (copy_len > rest)
					copy_len = rest;
				c_memcpy(ctrl->buf+ctrl->buf_len, &ctrl->tmp_buf[ctrl->tmp_len], copy_len);
				ctrl->buf_len += copy_len;
			}

			ctrl->tmp_len += read_len;

			// only for logging
			while (1) {
				int idx = c_strchr_idx(ctrl->tmp_buf, '\n', ctrl->tmp_size);
				if (idx == 0) {
					idx++;
					c_memcpy(ctrl->tmp_buf, &ctrl->tmp_buf[idx], ctrl->buf_len-idx + 1);  // including 0
					ctrl->buf_len -= idx;
				}
				else if (idx > 0) {
					ctrl->tmp_buf[idx] = 0;
					grm_log_debug("%s : %s", ctrl->caller, ctrl->tmp_buf);
					idx++;
					c_memcpy(ctrl->tmp_buf, &ctrl->tmp_buf[idx], ctrl->buf_len-idx + 1);  // including 0
					ctrl->buf_len -= idx;
				}
				else {
					if (ctrl->tmp_len >= ctrl->tmp_size) {
						grm_log_debug("%s : %s", ctrl->caller, ctrl->tmp_buf);
						c_memset(ctrl->tmp_buf, 0, sizeof(ctrl->tmp_buf));
						ctrl->tmp_len = 0;
					}
					break;
				}
			}  // loop for log
	}

	grm_log_debug("%s() : %s : out of thread",  __FUNCTION__, ctrl->caller);

/*
	if (ctrl->fp) {
		 (run_cmd_lock);
		fp = ctrl->fp;
		pid = ctrl->pid;
		ctrl->fp = NULL;
		ctrl->pid = 0;
		G_UNLOCK (run_cmd_lock);

		sys_pclose(fp, pid);
	}
*/

	ctrl->on_thread = FALSE;

	grm_log_debug("%s() : %s : stop thread : pid = %d", __FUNCTION__, ctrl->caller, pid);

	return NULL;
}


gboolean sys_run_cmd(gchar *cmd, long wait, char *logstr, char *output, int size)
{
	gboolean done = FALSE;
	RunCmdCtrl ctrl;
	pid_t	pid;
	FILE	*fp;

	if (c_strlen(cmd, 1024) > 0 ) {
		if (c_strlen(logstr, 256) <= 0 )
			logstr = "sys_run_cmd";

		c_memset(&ctrl, 0, sizeof(ctrl));
		ctrl.cmd = cmd;
		ctrl.buf = output;
		ctrl.buf_size = size;
		ctrl.caller = logstr;


//	fp = sys_popen(cmd, "r", &pid);
		fp = popen(cmd, "r");
		if (fp == NULL) {
			grm_log_debug("%s() : %s : can't run - %s", __FUNCTION__, logstr, cmd);
			return FALSE;
		}
		int fd = fileno(fp);
		if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
			grm_log_debug("%s() : %s : fcntl() : not set NONBLOCK: %s", __FUNCTION__, logstr, strerror(errno));
//			sys_pclose(fp, pid);
			pclose(fp);
			return FALSE;
		}
		grm_log_debug("%s() : %s : created child process  = %d",  __FUNCTION__, logstr, pid);

		ctrl.fp = fp;
		ctrl.fd = fd;
		ctrl.pid = pid;

		pthread_t thread;
		int res = pthread_create(&thread, NULL, run_cmd_thread, (void*)&ctrl);
		if (res != 0) {
			grm_log_error("%s : can't create thread : %s", __FUNCTION__, cmd);
			return FALSE;
		}
		done = TRUE;

		while (ctrl.on_thread == FALSE);

		long total = 0;
		while (ctrl.on_thread) {
			if (total >= wait)
				break;
			usleep(1000);
			total += 1000;
		}

		grm_log_debug("%s : request stop thread : %s", __FUNCTION__, cmd);

		ctrl.stop = TRUE;
		if (fp) {
//			G_LOCK (run_cmd_lock);
			ctrl.fp = NULL;
			ctrl.pid = 0;
//			G_UNLOCK (run_cmd_lock);
//			sys_pclose(fp, pid);
			pclose(fp);
		}

/*
		while (ctrl.on_thread) {
			usleep(1000);
		}
*/
	}

	grm_log_debug("%s : %s : end result = %d", __FUNCTION__, cmd, (int)done);

	return done;
}




