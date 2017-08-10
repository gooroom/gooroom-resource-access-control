/*
 * sys_file.h
 *
 *  Created on: 2015. 11. 19.
 *      Author: user
 */

#ifndef _SYS_FILE_H_
#define _SYS_FILE_H_

#include <glib.h>

gboolean sys_file_get_default_home_dir(gchar *user, gchar *homeDir, int size);
gboolean sys_file_change_owner_group(gchar *path, gchar* owner, gchar* group);
gboolean sys_file_get_owner_group(gchar *path, gchar *owner, int oSize,
		                                           gchar *group, int gSize);

gboolean sys_file_get_default_usb_dir(gchar *user, gchar *homeDir, int size);

gboolean sys_file_set_mode(gchar *path, gchar *mode);
gboolean sys_file_set_mode_user (gchar *path, gchar *mode);	// process only owner
gboolean sys_file_set_mode_group(gchar *path, gchar *mode); // process only group
gboolean sys_file_set_mode_other(gchar *path, gchar *mode); // process only other

gboolean sys_file_make_new_directory(gchar *path, gchar* owner, gchar* group, int mode);  // 존재하면 아무 동작 없음

//gboolean sys_file_get_mode(gchar *path, gchar *mode);		// mode;s size is 10 at least
//gboolean sys_file_get_mode_user (gchar *path, gchar *mode);		// mode;s size is 4 at least
//gboolean sys_file_get_mode_group(gchar *path, gchar *mode);		// mode;s size is 4 at least
//gboolean sys_file_get_mode_other(gchar *path, gchar *mode);		// mode;s size is 4 at least

#endif /* _SYS_FILE_H_ */
