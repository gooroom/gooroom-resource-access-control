/*
 * sys_comm.h
 *
 *  Created on: 2016. 6. 13.
 *      Author: yang
 */

#ifndef _SYS_COMM_H_
#define _SYS_COMM_H_

#include <glib.h>

typedef struct _SysComm SysComm;

SysComm* 	sys_comm_alloc();
void	  	sys_comm_free(SysComm **pcomm);

gboolean	sys_comm_uds_connect (SysComm* comm, gchar *path);
gboolean	sys_comm_uds_listen  (SysComm* comm, gchar *path);
gboolean	sys_comm_uds_accepted(SysComm* comm, int sd);

gboolean	sys_comm_socket_connect (SysComm* comm, gchar *addr, int port);
gboolean	sys_comm_socket_listen  (SysComm* comm, int port);
gboolean	sys_comm_socket_accepted(SysComm* comm, int sd);

void		sys_comm_close (SysComm *comm);

gboolean	sys_comm_set_nonblock(SysComm* comm);

gboolean	sys_comm_is_closed(SysComm *comm);
gboolean	sys_comm_check_readable(SysComm *comm, long usec);

int			sys_comm_get_sd(SysComm *comm);			// if error : -1

gint		sys_comm_read(SysComm *comm, gchar* buf, gint buf_size);
gint		sys_comm_read_line(SysComm *comm, gchar* buf, gint buf_size);	// until EOL or buf_size
gint		sys_comm_write(SysComm *comm, gchar* buf, gint len);


#endif /* SYS_COMM_H_ */
