/*
 * sys_comm.c
 *
 *  Created on: 2016. 6. 13.
 *      Author: yang
 */

/**
  @file 	 	sys_comm.c
  @brief		 socket 통신 관련 기능
  @details	class SysComm 정의 (GTK style의 c언어를 이용한 객체처리 사용) \n
  				일반 소켓 통신과 Unix Domain Spcket 통신을 지원한다.	\n
  				헤더파일 :  	sys_comm.h	\n
  				라이브러리:	libgrac.so
 */


#include "sys_comm.h"
#include "grm_log.h"
#include "cutility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/un.h>			//---
#include <sys/socket.h>	//---
#include <sys/stat.h>
#include <errno.h>


#define SYS_COMM_KIND_NONE		0
#define SYS_COMM_KIND_SOCKET	1	// general socket
#define SYS_COMM_KIND_UDS		  2	// Unix Domain Socket

#define SYS_COMM_BUF_SIZE	1024
#define SYS_COMM_BUF_MAX	1024*10  // 10K

struct _SysComm {
	int		kind;
	gchar	uds_path[256];	// max 108
	int		socket;

	gchar	addr[256];
	int		port;

	gboolean	opened;
	gboolean  accept_mode;
	gboolean  listen_mode;

	gboolean	readable;
//	int			error;

	char	*recv_buf;
	gint		recv_len;
	gint		recv_alloc_len;

/*
	char	send_buf[SYS_COMM_BUF_SIZE];
	int		send_len;
*/
};

//G_LOCK_DEFINE_STATIC (sys_comm_read_lock);

/**
 @brief   SysComm 객체 생성
 @details
      객체 지향형 언어의 생성자에 해당한다.
 @return 생성된 객체 주소
 */
SysComm* 	sys_comm_alloc()
{
	SysComm* comm = malloc(sizeof(SysComm));

	if (comm) {
		memset(comm, 0, sizeof(SysComm));

		comm->socket = -1;

		comm->recv_buf = malloc(SYS_COMM_BUF_SIZE);
		if (comm->recv_buf == NULL) {
			sys_comm_free(&comm);
		}
		else {
			comm->recv_alloc_len = SYS_COMM_BUF_SIZE;
			comm->recv_len = 0;
			memset(comm->recv_buf, 0, comm->recv_alloc_len);
		}
	}

	return comm;
}

/**
 @brief   SysComm 객체 해제
 @details
      객체 지향형 언어의 소멸자에  해당한다. 단 자동으로 불려지지 않고 필요한 시점에서 직접 호출하여야 한다.\n
      해제된  객체 주소값으로 0으로 설정하기 위해 이중 포인터를 사용한다.
 @param [in,out]  pcomm  해제하고자 하는 객체 주소를 담는 변수의 주소 (double pointer)
 */
void	  	sys_comm_free(SysComm **pcomm)
{
	if (pcomm) {
		SysComm* comm = *pcomm;
		if (comm) {
			c_free(&comm->recv_buf);
			free(comm);
		}
		*pcomm = NULL;
	}
}

/**
  @brief   Unix Domain Socket을 이용한 서버로의 접속 시도
  @details
  		접속에 사용하는 path의 permission에 의해 접속이 안되는 경우도 있으니 주의한다.
  @param [in]  comm	SysComm 객체주소
  @param [in]  path		Unix Domain Socket 통신에 사용할 경로
  @return 	gboolean	성공 여부
 */
gboolean	sys_comm_uds_connect(SysComm* comm, gchar *path)
{
	gboolean ret = FALSE;

	if (comm) {
		if (comm->opened)
			sys_comm_close(comm);

		int		sd;
		struct sockaddr_un  server_addr;

		sd = socket(PF_FILE, SOCK_STREAM, 0);
		if (sd == -1) {
			grm_log_debug("Debug: sys_comm_uds_connect(): can't create socket");
		}
		else {
			memset(&server_addr, 0, sizeof(server_addr));
			server_addr.sun_family = AF_UNIX;
			strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path));
			int n = connect(sd, (struct sockaddr*)&server_addr, sizeof(server_addr));
			if (n == -1) {
				grm_log_debug("sys_comm_uds_connect(): can't connect : %s", strerror(errno));
			}
			else {
				comm->kind = SYS_COMM_KIND_UDS;
				strncpy(comm->uds_path, path, sizeof(comm->uds_path));
				comm->socket = sd;
				comm->opened = TRUE;
				ret = TRUE;
			}
		}
	}

	return ret;
}

/**
  @brief   Unix Domain Socket을 이용한 클라이언트로부터의 접속 요청 대기
  @details
  		접속에 사용하는 path의 permission에 의해 접속이 안되는 경우도 있으니 주의한다.
  @param [in]  comm	SysComm 객체주소
  @param [in]  path		Unix Domain Socket 통신에 사용할 경로
  @return 	gboolean	성공 여부
 */
gboolean	sys_comm_uds_listen(SysComm* comm, gchar *path)
{
	gboolean ret = FALSE;
	char *func = "sys_comm_uds_listen()";

	if (comm) {
		int		sd;
		struct sockaddr_un server_addr;

		if (access(path, F_OK) == 0) {
			if (unlink(path) != 0) {
				grm_log_debug("%s : unlink : %s", func, strerror(errno));
				return FALSE;
			}
		}
		strncpy(comm->uds_path, path, sizeof(comm->uds_path));

		sd = socket(PF_FILE, SOCK_STREAM, 0);
		if (sd == -1) {
			grm_log_debug("%s : can't create socket : %s", func, strerror(errno));
			return FALSE;
		}

		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sun_family = AF_UNIX;
		strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path));

		if (bind(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
			grm_log_debug("%s : can't bind : %s", func, strerror(errno));
			return FALSE;
		}

		comm->listen_mode = TRUE;
		if (access(path, F_OK) == 0) {
			grm_log_debug("%s : created : %s", func, path);
		}
		else {
			grm_log_debug("%s : no exist : %s", func, path);
		}

		if (listen(sd, 10) == -1) {
			grm_log_error("%s : listen : %s", func, strerror(errno));
			return FALSE;
		}

		chmod(path, 0777);	// Notice!!!!

		comm->kind = SYS_COMM_KIND_UDS;
		comm->socket = sd;
		comm->opened = TRUE;
		ret = TRUE;

	}
	return ret;
}


/**
  @brief   Unix Domain Socket을 이용한 클라이언트로부터의 접속 요청을 허락한 socket descriptor 설정
  @details
  		accept()를 위한 loop는 SysComm 외부에서 구현하고 accept()된  socket descriptor를 전달한다.
  @param [in]  comm	SysComm 객체주소
  @param [in]  sd			accept된 sokcet descriptor
  @return 	gboolean	성공 여부
 */
gboolean	sys_comm_uds_accepted   (SysComm* comm, int sd)
{
	gboolean ret = FALSE;

	if (comm) {
		comm->kind = SYS_COMM_KIND_UDS;
		comm->socket = sd;
		comm->accept_mode = TRUE;
		comm->opened = TRUE;
		ret = TRUE;
	}

	return ret;
}

/**
  @brief   일반 socket을 이용한 서버로의 접속 시도
  @param [in]  comm	SysComm 객체주소
  @param [in]  addr		서버 주소
  @param [in]  port		포트 번호
  @return 	gboolean	성공 여부
 */
gboolean	sys_comm_socket_connect (SysComm* comm, gchar *addr, int port)
{
	gboolean ret = FALSE;

	if (comm) {
		if (comm->opened)
			sys_comm_close(comm);

		int		sd;
		struct sockaddr_in  server_addr;

		sd = socket(AF_INET, SOCK_STREAM, 0);
		if (sd == -1) {
			grm_log_debug("Debug: sys_comm_socket_conect(): can't create socket");
		}
		else {
			memset(&server_addr, 0, sizeof(server_addr));
			server_addr.sin_family = AF_INET;
			server_addr.sin_port = htons(port);
			server_addr.sin_addr.s_addr = inet_addr(addr);
			if (server_addr.sin_addr.s_addr == -1) {
				struct hostent *host = gethostbyname(addr);
				if (host == NULL) {
					grm_log_debug("Debug: sys_comm_socket_conect(): can't get hostname");
					return FALSE;
				}
				bcopy (host->h_addr_list[0], (char*)&server_addr.sin_addr, host->h_length);
			}

			int n = connect(sd, (struct sockaddr*)&server_addr, sizeof(server_addr));
			if (n == -1) {
				grm_log_debug("sys_comm_socket_connect(): can't connect : %s", strerror(errno));
			}
			else {
				comm->kind = SYS_COMM_KIND_SOCKET;
				strncpy(comm->addr, addr, sizeof(comm->addr));
				comm->port = port;
				comm->socket = sd;
				comm->opened = TRUE;
				ret = TRUE;
			}
		}
	}

	return ret;

}

/**
  @brief   일반 socket을 이용한 클라이언트로부터의 접속 요청 대기
  @details
  			특정 주소가 아닌 모든 주소로부터의 접속 요청을 기다린다.
  @param [in]  comm	SysComm 객체주소
  @param [in]  port		포트번호
  @return 	gboolean	성공 여부
 */
gboolean	sys_comm_socket_listen  (SysComm* comm, int port)
{
	gboolean ret = FALSE;
	char *func = "sys_comm_uds_listen()";

	if (comm) {
		int		sd;
		struct sockaddr_in server_addr;

		sd = socket(AF_INET, SOCK_STREAM, 0);
		if (sd == -1) {
			grm_log_debug("%s : can't create socket : %s", func, strerror(errno));
			return FALSE;
		}

		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(port);

		if (bind(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
			grm_log_debug("%s : can't bind : %s", func, strerror(errno));
			return FALSE;
		}

		comm->listen_mode = TRUE;

		if (listen(sd, 10) == -1) {
			grm_log_error("%s : listen : %s", func, strerror(errno));
			return FALSE;
		}

		comm->kind = SYS_COMM_KIND_SOCKET;
		comm->socket = sd;
		comm->opened = TRUE;
		ret = TRUE;

	}
	return ret;
}

/**
  @brief   일반 socket을 이용한 클라이언트로부터의 접속 요청을 허락한 socket descriptor 설정
  @details
  		accept()를 위한 loop는 SysComm 외부에서 구현하고 accept()된  socket descriptor를 전달한다.
  @param [in]  comm	SysComm 객체주소
  @param [in]  sd			accept된 sokcet descriptor
  @return 	gboolean	성공 여부
 */
gboolean	sys_comm_socket_accepted(SysComm* comm, int sd)
{
	gboolean ret = FALSE;

	if (comm) {
		comm->kind = SYS_COMM_KIND_SOCKET;
		comm->socket = sd;
		comm->accept_mode = TRUE;
		comm->opened = TRUE;
		ret = TRUE;
	}

	return ret;
}

/**
  @brief   접속을 끊는다.
  @details
  		이 함수는 socket의 유형을 구분하지 않는다.
  @param [in]  comm	SysComm 객체주소
 */
void		sys_comm_close (SysComm *comm)
{
	if (comm) {
		if (comm->opened) {
			close(comm->socket);
			comm->opened = FALSE;
		}

		if (comm->kind == SYS_COMM_KIND_UDS && comm->listen_mode) {
			unlink(comm->uds_path);
		}

		comm->listen_mode = FALSE;
		comm->accept_mode = FALSE;


		comm->readable = FALSE;
	}
	//grm_log_debug("sys_comm_close()");
}

/**
  @brief   접속이 끊어졌는지 확인한다.
  @details
  		이 함수는 socket의 유형을 구분하지 않는다.
  @param [in]  comm	SysComm 객체주소
  @return gboolean  TRUE:접속 끊김
 */
gboolean	sys_comm_is_closed(SysComm *comm)
{
	gboolean res = TRUE;

	if (comm && comm->opened)
		res = FALSE;

	return res;
}

/**
  @brief   통신 방식을 Non-Blocking mode로 설정한다.
  @details
  		이 함수는 socket의 유형을 구분하지 않는다.
  @param [in]  comm	SysComm 객체주소
  @return gboolean  성공 여부
 */
gboolean	sys_comm_set_nonblock(SysComm* comm)
{
	gboolean done = FALSE;

	if (comm) {
		switch (comm->kind) {
		case SYS_COMM_KIND_SOCKET	:
		case SYS_COMM_KIND_UDS		:
			if (comm->socket >= 0) {
				int flags, res;
				flags = fcntl(comm->socket, F_GETFL, 0);
				flags |= O_NONBLOCK;
				if (flags != -1) {
					res = fcntl(comm->socket, F_SETFL, flags);
					if (res >= 0)
						done = TRUE;
				}
			}
			break;
		}
	}

	return done;
}

/**
  @brief   객체에서 사용 중인 socket descriptor를 얻는다.
  @details
  		이 함수는 socket의 유형을 구분하지 않는다.
  @param [in]  comm	SysComm 객체주소
  @return int  socket descriptor,  오류시는 -1
 */
int	 sys_comm_get_sd(SysComm *comm)
{
	int sd = -1;
	if (comm)
		sd = comm->socket;

	return sd;
}

// 수신된 데이터기 있는지 확인
static gboolean _check_comm_readable(SysComm *comm, long usec)
{
	gboolean ret = FALSE;

	if (comm == NULL)
		return FALSE;

	if (comm->opened) {
		struct timeval tm_out;
		struct timeval *p_tm_out;

		if (usec >= 0) {
			memset(&tm_out, 0, sizeof(tm_out));
			tm_out.tv_sec = 0;
			tm_out.tv_usec = usec;
			p_tm_out = &tm_out;
		}
		else {
			p_tm_out = NULL;
		}

		fd_set test_sets;
		FD_ZERO(&test_sets);
		FD_SET(comm->socket, &test_sets);
		int n = select(comm->socket+1, &test_sets, NULL, NULL, p_tm_out);
		if (n < 0) {
			grm_log_debug("sys_comm_is_readble(): select : %s", strerror(errno));
		}
		else if ( n > 0) {
			if (FD_ISSET(comm->socket, &test_sets))
				ret = TRUE;
		}
	}

	return ret;
}


/**
  @brief   수신할 데이터가 있는지 확인한다.
  @details
  		이 함수는 socket의 유형을 구분하지 않는다.
  @param [in]  comm	SysComm 객체주소
  @param [in]  usec		대기 시간 (micro second)
  @return gboolean	데이터 존재 여부
 */
gboolean	sys_comm_check_readable(SysComm *comm, long usec)
{
	gboolean ret = FALSE;

	if (comm == NULL)
		return FALSE;

	if (comm->recv_len > 0)
		return TRUE;

	if (comm->readable)
		return TRUE;

	ret = _check_comm_readable(comm, usec);

	comm->readable = ret;

	return ret;
}

// 데이터 수신
static int	_comm_read(SysComm *comm)
{
	int 	n = 0;

	if (comm == NULL)
		return 0;

	if (comm->opened) {
		gboolean dataB = _check_comm_readable(comm, 0L);
		int rest;
		while (dataB) {
			rest = comm->recv_alloc_len - comm->recv_len;
			if (rest < 256) {
				int alloc_len = comm->recv_alloc_len + SYS_COMM_BUF_SIZE;
				if (alloc_len < SYS_COMM_BUF_MAX) {
					char *ptr = malloc(alloc_len);
					if (ptr) {
						grm_log_debug("realloc packet buffer to recv");
						memset(ptr, 0, alloc_len);
						memcpy(ptr, comm->recv_buf, comm->recv_len);
						comm->recv_alloc_len = alloc_len;
						c_free(&comm->recv_buf);
						comm->recv_buf = ptr;
					}
				}
			}
			rest = comm->recv_alloc_len - comm->recv_len - 1;
			if (rest <= 0)
				break;

			n = read(comm->socket, comm->recv_buf + comm->recv_len, rest);
			if (n == 0) {
				close(comm->socket);
				comm->opened = FALSE;
				break;
			}
			else if (n < 0) {
				grm_log_debug("sys_comm_read(): read error : %s", strerror(errno));
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					n = 0;
				}
				else {
					break;
				}
			}
			else {
				grm_log_debug("sys_comm_read(): %s", comm->recv_buf + comm->recv_len);
				comm->recv_len += n;
			}
			dataB = _check_comm_readable(comm, 0L);
		}
	}
	comm->readable = FALSE;

	return comm->recv_len;
}

/**
  @brief   데이터를 수신한다.
  @details
  		이 함수는 socket의 유형을 구분하지 않는다.
  @param [in] 	 comm		SysComm 객체주소
  @param [out]  buf			데이터를 보관할 버퍼
  @param [in]  	 buf_size	버퍼 크기
  @return int	읽어들인 데이터 수 (바이트 단위)
 */
gint		sys_comm_read(SysComm *comm, gchar* buf, gint buf_size)
{
	gint 	n = 0;

	if (comm == NULL)
		return 0;

	_comm_read(comm);

	if (comm->recv_len > 0) {
		if (comm->recv_len >= buf_size-1 )
			n = buf_size-1;
		else
			n = comm->recv_len;
		memcpy(buf, comm->recv_buf, n);
		buf[n] = 0;

		if ( n >= comm->recv_len ) {
			memset(comm->recv_buf, 0, n);
			comm->recv_len = 0;
		}
		else {
			int rest = comm->recv_len - n;
			memmove(comm->recv_buf, comm->recv_buf+n, rest);
			memset(comm->recv_buf+rest, 0, n);
			comm->recv_len = rest;
		}
	}

	return n;
}

/**
  @brief   한 라인의 데이터를 수신한다.
  @details
  		이 함수는 socket의 유형을 구분하지 않는다. \n
  		라인은 0x0a로 구분된다.
  @param [in] 	 comm		SysComm 객체주소
  @param [out]  buf			데이터를 보관할 버퍼
  @param [in]  	 buf_size	버퍼 크기
  @return int	읽어 들인 데이터 수 (바이트 단위)
 */
gint		sys_comm_read_line(SysComm *comm, gchar* buf, gint buf_size)
{
	gint 	n = 0;

	if (comm == NULL)
		return 0;

	_comm_read(comm);

	if (comm->recv_len > 0) {
		gint idx;

		for (idx=0; idx<comm->recv_len; idx++) {
			if (comm->recv_buf[idx] == '\n')
				break;
		}

		if (idx >= buf_size-1) {
			n = buf_size-1;
		}
		else {
			if (idx == comm->recv_len)  // not found EOL
				n = 0;
			else
				n = idx+1;	// including EOL
		}

		if (n > 0) {
			memcpy(buf, comm->recv_buf, n);
			buf[n] = 0;

			if ( n >= comm->recv_len ) {
				memset(comm->recv_buf, 0, n);
				comm->recv_len = 0;
			}
			else {
				int rest = comm->recv_len - n;
				memmove(comm->recv_buf, comm->recv_buf+n, rest);
				memset(comm->recv_buf+rest, 0, n);
				comm->recv_len = rest;
			}
		}
	}

	return n;
}


/**
  @brief   데이터를 전송한다.
  @details
  		이 함수는 socket의 유형을 구분하지 않는다.
  @param [in] 	 comm		SysComm 객체주소
  @param [in]	 buf			전송할 데이터
  @param [in]  	 len			데이터 길이
  @return int	 전송한 데이터 수 (바이트 단위)
 */
gint		sys_comm_write(SysComm *comm, gchar* buf, gint len)
{
	gint n = 0;

	if (comm && comm->opened) {
		n = write(comm->socket, buf, len);
		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				n = 0;
			grm_log_debug("sys_comm_write(): write error : %s", strerror(errno));
		}
	}

	return n;
}

