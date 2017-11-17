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
 * grac_rule_hook.c
 *
 *  Created on: 2017. 8. 16.
 *      Author: gooroom@gooroom.kr
 */

/**
  @file 	 	grac_rule_hook.c
  @brief		각 디바이스제어를 위한 hooking 관련 함수들
  @details
  				헤더파일 :  	grac_rule_hook.h	\n
  				라이브러리:	libgrac-device.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "grac_rule_hook.h"
#include "grac_config.h"
#include "grac_log.h"
#include "cutility.h"
#include "sys_user.h"
#include "sys_file.h"

static gboolean _create_ld_so_preload()
{
	gboolean done = TRUE;
	const char* hook_clip_path;
	const char* hook_screen_path;
	const char* preload_link;
	char	preload_target[2048];
	int		uid;

	uid = sys_user_get_login_uid();
	if (uid < 0) {
		grac_log_error("%s(): can't get login name", __FUNCTION__);
		return FALSE;
	}
	g_snprintf(preload_target, sizeof(preload_target), "/var/run/user/%d/%s", uid, grac_config_file_ld_so_preload());

	preload_link = grac_config_path_ld_so_preload();
	if (preload_link == NULL) {
		grac_log_error("%s(): undefined preload path", __FUNCTION__);
		return FALSE;
	}

	hook_clip_path = grac_config_path_hook_clipboard_so();
	if (hook_clip_path == NULL) {
		grac_log_error("%s(): undefined hook module for clipboard", __FUNCTION__);
		done = FALSE;
	}
	hook_screen_path = grac_config_path_hook_screenshooter_so();
	if (hook_screen_path == NULL) {
		grac_log_error("%s(): undefined hook module for screenshooter", __FUNCTION__);
		done = FALSE;
	}

	FILE	*fp;
	char	buf[2048];
	gboolean exist_clip_path = FALSE;
	gboolean exist_screen_path = FALSE;
	char	last_char = '\n';

	fp = g_fopen(preload_target, "r");
	if (fp != NULL) {
		while (1) {
			if (fgets(buf, sizeof(buf), fp) == NULL)
				break;
			int n = c_strlen(buf, sizeof(buf));
			if (n == 0)
				continue;
			last_char = buf[n-1];
			c_strtrim(buf, sizeof(buf));

			if (c_strmatch(buf, hook_clip_path))
				exist_clip_path = TRUE;
			if (c_strmatch(buf, hook_screen_path))
				exist_screen_path = TRUE;
		}
		fclose(fp);
	}

	if (exist_clip_path == FALSE || exist_screen_path == FALSE) {
		fp = g_fopen(preload_target, "a");
		if (fp != NULL) {
			if (last_char != '\n')
				fprintf(fp, "\n");
			if (exist_clip_path == FALSE && hook_clip_path != NULL)
				fprintf(fp, "%s\n", hook_clip_path);
			if (exist_screen_path == FALSE && hook_screen_path != NULL)
				fprintf(fp, "%s\n", hook_screen_path);
			fclose(fp);
		} else {
			grac_log_error("%s(): can't open to add the hook module [%s]", __FUNCTION__, preload_target);
			done = FALSE;
		}
	}

	if (done) {
		int res;
		unlink(preload_link);
		res = symlink(preload_target, preload_link);
		if (res != 0) {
			grac_log_error("%s(): can't make link : %s", __FUNCTION__, strerror(errno) );
			done = FALSE;
		}
	}

	return done;
}

static gboolean _delete_ld_so_preload()
{
	gboolean done = TRUE;
	const char* hook_clip_path;
	const char* hook_screen_path;
	const char* preload_path;

	preload_path = grac_config_path_ld_so_preload();
	if (preload_path == NULL) {
		grac_log_error("%s(): undefined preload path", __FUNCTION__);
		return FALSE;
	}

	hook_clip_path = grac_config_path_hook_clipboard_so();
	if (hook_clip_path == NULL) {
		grac_log_error("%s(): undefined hook module for clipboard", __FUNCTION__);
		done = FALSE;
	}
	hook_screen_path = grac_config_path_hook_screenshooter_so();
	if (hook_screen_path == NULL) {
		grac_log_error("%s(): undefined hook module for screenshooter", __FUNCTION__);
		done = FALSE;
	}

	char	tmp_file[2048];
	FILE	*in_fp, *out_fp;
	gboolean out_data = FALSE;

	g_snprintf(tmp_file, sizeof(tmp_file), "%s/grac_preload.tmp", (char*)grac_config_dir_grac_data());
	in_fp = g_fopen(preload_path, "r");
	if (in_fp == NULL)
		return done;

	out_fp = g_fopen(tmp_file, "w");
	if (out_fp != NULL) {
		char	buf1[2048];
		char	buf2[2048];
		while (1) {
			if (fgets(buf1, sizeof(buf1), in_fp) == NULL)
				break;
			c_strcpy(buf2, buf1, sizeof(buf2));
			c_strtrim(buf2, sizeof(buf2));
			if (c_strlen(buf2, sizeof(buf2)) <= 0)
				continue;

			if (c_strmatch(buf2, hook_clip_path))
				continue;
			if (c_strmatch(buf2, hook_screen_path))
				continue;

			out_data = TRUE;
			fprintf(out_fp, "%s", buf1);
		}
		fclose(out_fp);
	} else {
		grac_log_error("%s(): can't create tmp file for preload [%s]", __FUNCTION__, tmp_file);
		done = FALSE;
	}
	fclose(in_fp);

	if (done) {
		if (out_data) {
			unlink(preload_path);
			rename(tmp_file, preload_path);
		} else {
			unlink(preload_path);
			unlink(tmp_file);
		}
	}

	return done;
}

gboolean grac_rule_hook_init()
{
	gboolean done = TRUE;
	const char* path;

	path = grac_config_path_hook_clipboard_so();
	if (path == NULL) {
		grac_log_error("%s(): undefined hook module for clipboard", __FUNCTION__);
		done = FALSE;
	} else if (sys_file_is_existing((char*)path) == FALSE) {
		grac_log_error("%s(): not found hook module [%s]", __FUNCTION__, path);
		done = FALSE;
	}

	path = grac_config_path_hook_screenshooter_so();
	if (path == NULL) {
		grac_log_error("%s(): undefined hook module for screenshooter", __FUNCTION__);
		done = FALSE;
	} else if (sys_file_is_existing((char*)path) == FALSE) {
		grac_log_error("%s(): not found hook module [%s]", __FUNCTION__, path);
		done = FALSE;
	}

	if (done)
		done = _create_ld_so_preload();

	return done;
}

gboolean grac_rule_hook_clear()
{
	const char *path;

	_delete_ld_so_preload();

	path = grac_config_path_hook_clipboard_conf();
	if (path)
		unlink(path);
	path = grac_config_path_hook_screenshooter_conf();
	if (path)
		unlink(path);

	return TRUE;
}

gboolean grac_rule_hook_allow_clipboard(gboolean allow)
{
	gboolean done = FALSE;
	const char* path;

	path = grac_config_path_hook_clipboard_conf();
	if (path) {
		FILE	*fp;
		fp = g_fopen(path, "w");
		if (fp == NULL) {
			grac_log_error("%s(): can't open [%s]", __FUNCTION__, path);
		} else {
			if (allow)
				fprintf(fp, "0\n");
			else
				fprintf(fp, "1\n");
			fclose(fp);
			done = TRUE;
		}
	}

	return done;
}

gboolean grac_rule_hook_allow_screenshooter(gboolean allow)
{
	gboolean done = FALSE;
	const char* path;

	path = grac_config_path_hook_screenshooter_conf();
	if (path) {
		FILE	*fp;
		fp = g_fopen(path, "w");
		if (fp == NULL) {
			grac_log_error("%s(): can't open [%s]", __FUNCTION__, path);
		} else {
			if (allow)
				fprintf(fp, "0\n");
			else
				fprintf(fp, "1\n");
			fclose(fp);
			done = TRUE;
		}
	}

	return done;
}

