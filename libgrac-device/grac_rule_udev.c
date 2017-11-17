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
 * grac_rule_udev.c
 *
 *  Created on: 2017. 8. 16.
 *      Author: gooroom@gooroom.kr
 */

/**
  @file 	 	grac_rule_udev.c
  @brief		각 디바이스제어를 위한 udev rule 관련 함수들
  @details
  				헤더파일 :  	grac_apply_udev.h	\n
  				라이브러리:	libgrac-device.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "grac_rule_udev.h"
#include "grac_log.h"
#include "cutility.h"

typedef enum {
	GRAC_RULE_MAP_LINE_ERROR = -1,
	GRAC_RULE_MAP_LINE_EMPTY = 0,
	GRAC_RULE_MAP_LINE_COMMENT,
	GRAC_RULE_MAP_LINE_RULE,
	GRAC_RULE_MAP_LINE_MAPINFO,
	GRAC_RULE_MAP_LINE_SPECIAL
} grac_rule_map_line_t;

struct _GracRuleUdev
{
	char	*map_file_path;
	int		*line_status;
	int		line_max;
	int		line_count;
};

#define GRAC_RULE_MAP_KEY	"[grac_rule]"

// (1) map file syntax --> use a comment line
// # [grac-rule] resource permission
//
// (2) mapfile group -> separated by empty line
//     # comment
//     rule1
//     rule2
//     #rule3
//     empty line
//


/**
 @brief   GracRuleUdev  객체 생성
 @details
      	객체 지향형 언어의 생성자에 해당한다.
 @param [in]  map_file  udev rule 생성을 위한 map 설정 파일명
 @return GracRuleDev* 생성된 객체 주소
 */
GracRuleUdev* grac_rule_udev_alloc(const char *map_path)
{
	GracRuleUdev *rule = malloc(sizeof(GracRuleUdev));
	if (rule) {
		memset(rule, 0, sizeof(GracRuleUdev));
		if (map_path) {
			rule->map_file_path = c_strdup((char*)map_path, 2048);
			if (rule->map_file_path == NULL) {
				grac_log_debug("grac_rule_udev_alloc() : out of memory");
				grac_rule_udev_free(&rule);
				return NULL;
			}
		}
	}

	return rule;
}

/**
 @brief   GracRuleUdev 객체 해제
 @details
      객체 지향형 언어의 소멸자에  해당한다. 단 자동으로 불려지지 않고 필요한 시점에서 직접 호출하여야 한다.\n
      해제된  객체 주소값으로 0으로 설정하기 위해 이중 포인터를 사용한다.
 @param [in,out]  pAcc  해제하고자 하는 객체 주소를 담는 변수의 주소 (double pointer)
 */
void grac_rule_udev_free(GracRuleUdev **pRule)
{
	if (pRule != NULL && *pRule != NULL) {
		GracRuleUdev *rule = *pRule;

		c_free(&rule->map_file_path);

		free(rule->line_status);
		// rule->line_max = 0;
		// rule->line_count = 0;

		free(rule);

		*pRule = NULL;
	}
}


static gboolean	_parse_map_info_line(
		char *buf,   int buf_size,
		int *res_id, int *perm_id,
		char *opt,   int opt_size)
{
	gboolean done = FALSE;

	if (buf && res_id && perm_id && opt) {
		char *ptr;

		ptr = c_stristr(buf, GRAC_RULE_MAP_KEY, buf_size);
		if (ptr) {
			char word[256];
			int len;

			done = TRUE;

			// rule map keyword
			len = c_get_word(ptr, buf_size, NULL, word, sizeof(word));
			ptr += len;

			// resource name
			len = c_get_word(ptr, buf_size, NULL, word, sizeof(word));
			if (len > 0) {
				int id = grac_resource_get_resource_id(word);
				if (id >= 0)
					*res_id = id;
				else
					done = FALSE;
			} else {
				done = FALSE;
			}
			ptr += len;

			// permisssion
			len = c_get_word(ptr, buf_size, NULL, word, sizeof(word));
			if (len > 0) {
				int id = grac_resource_get_permission_id(word);
				if (id >= 0) {
					if (perm_id)
						*perm_id = id;
				} else {
					done = FALSE;
				}
			} else {
				done = FALSE;
			}
			ptr += len;

			len = c_get_word(ptr, buf_size, NULL, word, sizeof(word));
			if (len > 0) {
				c_strcpy(opt, word, opt_size);
			}	else {
				*opt = 0;
			}
		}  // if (ptr)
	}

	return done;
}

// map information line
//	- it is on the comment line
//	- syntax
//       # $map$  resource permission [option]
// comment line is starting from '#'
// Notice!!!
//   buf should be deleted leading and trailing spaces before calling this
//
// return -1:error 0:empty 1:normal 2:comment 3:map info 4: special(LABEL...)

static int	_check_line_kind(char *buf, int buf_size)
{
	grac_rule_map_line_t kind = GRAC_RULE_MAP_LINE_ERROR;

	if (buf) {

		kind = GRAC_RULE_MAP_LINE_EMPTY;

		int ch;
		ch = buf[0] & 0x0ff;
		if (ch == '#') {
			char word[256];
			int	len = c_get_word(buf+1, buf_size-1, NULL, word, sizeof(word));
			if (len > 0 && c_strimatch(word, GRAC_RULE_MAP_KEY))
				kind = GRAC_RULE_MAP_LINE_MAPINFO;
			else
				kind = GRAC_RULE_MAP_LINE_COMMENT;
		}	else if (ch > 0x20) {
			if (c_memcmp(buf, "LABEL=", 6, -1) == 0)
				kind = GRAC_RULE_MAP_LINE_SPECIAL;
			else
				kind = GRAC_RULE_MAP_LINE_RULE;
		}
	}

	return kind;
}


static gboolean _check_if_specified_rule(GracRule *rule, int res_id, int perm_id)
{
	gboolean check = FALSE;

	if (rule) {
		int set_perm_id = grac_rule_get_perm_id(rule, res_id);
		if (set_perm_id >= 0 && set_perm_id == perm_id) {
			check = TRUE;
		}
	}

	return check;
}

static int	get_line_count(char *file)
{
	FILE 	*fp;
	int		count = 0;

	fp = g_fopen(file, "r");
	if (fp != NULL) {
		char	buf[2048];
		while (1) {
			if (fgets(buf, sizeof(buf), fp) == NULL)
				break;
			count++;
		}
		fclose(fp);
	}

	return count;
}

static gboolean _analyze_map_line_status(GracRuleUdev* udev_rule, GracRule* grac_rule)
{
	FILE 	*fp;
	char	buf[1024];
	int		idx;
	int		rule_seq = 0;
	int 	map_on = 0;
	int		header_status = 0;	// 0: not start 1:collecting -1:end of header
	gboolean out_line;
	gboolean check = FALSE;

	if (udev_rule == NULL || grac_rule == NULL)
		return FALSE;

	fp = g_fopen(udev_rule->map_file_path, "r");
	if (fp == NULL)
		return FALSE;

	for (idx=0; idx < udev_rule->line_count; idx++) {
		if (fgets(buf, sizeof(buf), fp) == NULL)
				break;

		c_strtrim(buf, sizeof(buf));
		int kind = _check_line_kind(buf, sizeof(buf));

		out_line = FALSE;

		if (header_status == 0) {
			if (kind == GRAC_RULE_MAP_LINE_EMPTY) {
				out_line = TRUE;
			} else if (kind == GRAC_RULE_MAP_LINE_COMMENT) {
				out_line = TRUE;
				header_status = 1;
			}	else {
				header_status = -1;
			}
		} else if (header_status == 1) {
			if (kind == GRAC_RULE_MAP_LINE_EMPTY) {
				out_line = TRUE;
				header_status = -1;
			}	else if (kind == GRAC_RULE_MAP_LINE_COMMENT) {
				out_line = TRUE;
			}	else {
				header_status = -1;
			}
		}

		if (kind == GRAC_RULE_MAP_LINE_MAPINFO || kind == GRAC_RULE_MAP_LINE_SPECIAL)
	    {
			if (map_on == 0) {
				check = FALSE;
				rule_seq++;
			}
			map_on = 1;
		}

		if (map_on)
			udev_rule->line_status[idx] = rule_seq;
		else
			udev_rule->line_status[idx] = 0;
		if (kind == GRAC_RULE_MAP_LINE_COMMENT)
			udev_rule->line_status[idx] |= 0x4000;

		if (kind == GRAC_RULE_MAP_LINE_MAPINFO) {
			int res_id, perm_id;
			char opt[256];
			gboolean	res;
			res = _parse_map_info_line(buf, sizeof(buf), &res_id, &perm_id, opt, sizeof(opt));
			if (res == FALSE)
				grac_log_debug("invalid line : %s", buf);

			if (res) {
				check |= _check_if_specified_rule(grac_rule, res_id, perm_id);
				// adjust flag for previous lines
				if (check) {
					int prev;
					for (prev=idx-1; prev >= 0; prev--) {
						int pseq = udev_rule->line_status[prev] & 0x3fff;
						if (pseq == rule_seq || (udev_rule->line_status[prev] & 0x4000))
							udev_rule->line_status[prev] |= 0x8000;
						else
							break;
					}
				}
			}
		} else if (kind == GRAC_RULE_MAP_LINE_SPECIAL) {
			int prev;
			check = TRUE;
			for (prev=idx-1; prev >= 0; prev--) {
				int pseq = udev_rule->line_status[prev] & 0x3fff;
				if (pseq == rule_seq || (udev_rule->line_status[prev] & 0x4000))
					udev_rule->line_status[prev] |= 0x8000;
				else
					break;
			}
		}

		if (check && map_on)
			out_line = TRUE;

		// check and commnet & rule
		if (out_line)
			udev_rule->line_status[idx] |= 0x8000;

		if (kind == 0) {
			map_on = 0;
			check = FALSE;
		}
	}

	fclose(fp);

	return TRUE;
}

static void _out_udev_rule_file_header(FILE *fp)
{
	if (fp) {
		fprintf(fp, "#\n");
		fprintf(fp, "# Created by Gooroom OS : Gooroom Resource Access Control System\n");
		fprintf(fp, "#\n");
	}
}

// ATTR{uniq}=="00:15:83:d0:ef:25"
static gboolean _check_bluetooth_addr_line(char *buf, int bsize, char *format, int fsize)
{
	gboolean res = FALSE;
	// char	*subsystem = "SUBSYSTEM==\"bluetooth\"";
	char	*addr_key;
	char	*addr_key1 = "ATTR{uniq}==\"";
	char	*addr_key2 = "ATTRS{uniq}==\"";
	char	*ptr1;
	char	*ptr2;
	int		n, ch;

	// if (c_strstr(buf, subsystem, bsize) == NULL)
	// 	return FALSE;

	ptr1 = c_strstr(buf, addr_key1, bsize);
	if (ptr1 != NULL) {
		addr_key = addr_key1;
	} else {
		ptr1 = c_strstr(buf, addr_key2, bsize);
		if (ptr1 != NULL) {
			addr_key = addr_key2;
		} else {
			return FALSE;
		}
	}

	n = c_strlen(addr_key, 256);
	ptr1 += n;
	ptr2 = ptr1;
	while (1) {
		ch = *ptr2;
		if (ch == '\"')
			break;
		if (ch == 0)
			break;
		ptr2++;
	}

	if (ch != '\"')
		return FALSE;

	int	save_ch;
	save_ch = *(ptr1-1);
	*(ptr1-1) = 0;		// end mark on location of '\"'
	ptr2++;				    // move loaction after '\"'

	int fidx, i;
	fidx = 0;

	n = c_strlen(buf, fsize);
	for (i=0; i < n; i++) {
		if (buf[i] == '%') {
			if (fidx < fsize-2) {
				format[fidx++] = '%';
				format[fidx++] = '%';
			} else {
				break;
			}
		} else {
			if (fidx < fsize-1) {
				format[fidx++] = buf[i];
			} else {
				break;
			}
		}
	}
	format[fidx] = 0;
	if (i != n || buf[i] != 0) {
		*(ptr1-1) = save_ch;
		return FALSE;
	}
	*(ptr1-1) = save_ch;


	c_strcat(format, "\"%s\"", fsize);
	fidx = c_strlen(format, fsize);

	n = c_strlen(ptr2, fsize);
	for (i=0; i < n; i++) {
		if (ptr2[i] == '%') {
			if (fidx < fsize-2) {
				format[fidx++] = '%';
				format[fidx++] = '%';
			} else {
				break;
			}
		}	else {
			if (fidx < fsize-1) {
				format[fidx++] = ptr2[i];
			} else {
				break;
			}
		}
	}
	format[fidx] = 0;
	if (i != n || ptr2[i] != 0) {
		return FALSE;
	}

	res = FALSE;
	n = c_strlen(format, fsize);
	if (n > 0 && n < fsize) {
		if (format[n-1] != '\n') {
			if (n < fsize-1) {
				format[n] = '\n';
				format[n+1] = 0;
				res = TRUE;
			}
		} else {
			res = TRUE;
		}
	}

	return res;
}


// ATTR를 제거한다.
// ATTR{uniq}=="00:15:83:d0:ef:25"
gboolean _delete_bluetooth_addr_line(char *buf, int bsize, char *format, int fsize)
{
	gboolean res = FALSE;
	// char	*subsystem = "SUBSYSTEM==\"bluetooth\"";
	char	*addr_key;
	char	*addr_key1 = "ATTR{uniq}==\"";
	char	*addr_key2 = "ATTRS{uniq}==\"";
	char	*ptr1;
	char	*ptr2;
	int		n, ch;

// 	if (c_strstr(buf, subsystem, bsize) == NULL)
// 		return FALSE;

	// ATTRS가 없으므로 전체가 유효하다.
	ptr1 = c_strstr(buf, addr_key1, bsize);
	if (ptr1 != NULL) {
		addr_key = addr_key1;
	} else {
		ptr1 = c_strstr(buf, addr_key2, bsize);
		if (ptr1 != NULL) {
			addr_key = addr_key2;
		}
	}
	if (ptr1 == NULL) {
		n = c_strlen(buf, fsize);
		if (n < fsize-1) {
			c_strcpy(format, buf, fsize);
			return TRUE;
		} else {
			return FALSE;
		}
	}

	n = c_strlen(addr_key, 256);

	ptr2 = ptr1 + n;
	while (1) {
		ch = *ptr2;
		if (ch == '\"')
			break;
		if (ch == 0)
			break;
		ptr2++;
	}

	if (ch != '\"')
		return FALSE;

	ptr2++;
	while (1) {
		ch = *ptr2;
		if (ch == ',') {
			ptr2++;
			break;
		}
		if (ch == 0)
			break;
		if (ch > 0x20)
			break;
		ptr2++;
	}

	if (ch != 0 && ch != ',')
		return FALSE;

	int	save_ch;
	save_ch = *ptr1;
	*ptr1 = 0;
	if (ch == 0)
		g_snprintf(format, fsize, "%s", buf);
	else if (*ptr2 <= 0x20)
		g_snprintf(format, fsize, "%s%s", buf, ptr2);
	else
		g_snprintf(format, fsize, "%s %s", buf, ptr2);
	*ptr1 = save_ch;

	res = FALSE;
	n = c_strlen(format, fsize);
	if (n > 0 && n < fsize) {
		if (format[n-1] != '\n') {
			if (n < fsize-1) {
				format[n] = '\n';
				format[n+1] = 0;
				res = TRUE;
			}
		} else {
			res = TRUE;
		}
	}

	// adjust %%
	if (res == TRUE) {
		int i, cnt;
		n = c_strlen(format, fsize);
		cnt = 0;
		for (i=0; i < n; i++) {
			if (format[i] == '%')
				cnt++;
		}
		if (cnt > 0) {
			if (cnt + n > fsize-1) {
				res = FALSE;
			} else {
				for (i=n; i >= 0; i--) {
					if (format[i] == '%') {
						format[i+cnt] = '%';
						format[i+cnt-1] = '%';
						cnt--;
					} else {
						format[i+cnt] = format[i];
					}
					if (cnt == 0)
						break;
				}
			}
		}
	}

	return res;
}

static void _adjust_mac_addr(char *mac_addr)
{
	int	i, n;

	n = c_strlen(mac_addr, 256);
	for (i=0; i < n; i++) {
		int ch = mac_addr[i];
		if (ch == '.')
			mac_addr[i] = ':';
		else
			mac_addr[i] = tolower(ch);
	}
}


static gboolean _make_udev_rule_file(GracRuleUdev *udev_rule, GracRule* grac_rule, const char *rules_file)
{
	FILE	*in_fp;
	FILE	*out_fp;
	char	buf[2048];
	char	out_format[2048];

	if (udev_rule == NULL || rules_file == NULL)
		return FALSE;

	in_fp = g_fopen(udev_rule->map_file_path, "r");
	if (in_fp == NULL) {
		return FALSE;
	}

	out_fp = g_fopen(rules_file, "w");
	if (out_fp == NULL) {
		fclose(in_fp);
		return FALSE;
	}

	_out_udev_rule_file_header(out_fp);

	int rule_idx;
	int	out;
	// int	seq;

	for (rule_idx=0; rule_idx < udev_rule->line_count; rule_idx++) {
		if (fgets(buf, sizeof(buf), in_fp) == NULL)
				break;

		// seq = udev_rule->line_status[rule_idx] & 0x3fff;
		if (udev_rule->line_status[rule_idx] & 0x8000)
			out = 1;
		else
			out = 0;

		if (out) {
			if (_check_bluetooth_addr_line(buf, sizeof(buf), out_format, sizeof(out_format))) {
				int count;
				count = grac_rule_bluetooth_mac_count(grac_rule);
				/*
				if (count == 0) {	// MAC 주소 지정 부분을 제거한다 -> 모든  bluetooh가  대상이 된다
					// bluetooth 전체 장치를 위한 rule은 별도로 있다고 가정
					// _delete_bluetooth_addr_line(buf, sizeof(buf), out_format, sizeof(out_format));
					// g_fprintf(out_fp, "%s", out_format);
				}
				*/
				if (count > 0) {
					int		mac_idx;
					char	mac_addr[256];
					for (mac_idx=0; mac_idx < count; mac_idx++) {
						grac_rule_bluetooth_mac_get_addr(grac_rule, mac_idx, mac_addr, sizeof(mac_addr));
						_adjust_mac_addr(mac_addr);
						g_fprintf(out_fp, out_format, mac_addr);
					}
				}
			} else {
				g_fprintf(out_fp, "%s", buf);
			}
		}
	}

	fclose(in_fp);
	fclose(out_fp);

	return TRUE;
}


/**
  @brief  매체접근 제어 정보에 맞추어 udev rules file을 생성한다.
  @details
       매체접근 설정 값과 udev rule과의 관계를 정의한 mapping file을 이용한다.
  @return gboolean	성공여부
*/
gboolean grac_rule_udev_make_rules(GracRuleUdev *udev_rule, GracRule* grac_rule, const char *udev_rule_path)
{
	gboolean done = FALSE;

	if (udev_rule == NULL || grac_rule == NULL || udev_rule_path == NULL)
		return FALSE;

	udev_rule->line_count = get_line_count(udev_rule->map_file_path);
	if (udev_rule->line_count <= 0) {
		grac_log_error("grac_rule_udev_make_rules() : not founed or invalid map file");
		return FALSE;
	}

	udev_rule->line_status = malloc(udev_rule->line_count * sizeof(int));
	if (udev_rule->line_status == NULL) {
		grac_log_error("grac_rule_udev_make_rules() : out of memory");
		return FALSE;
	}
	c_memset(udev_rule->line_status, 0, udev_rule->line_count * sizeof(int));

	done = _analyze_map_line_status(udev_rule, grac_rule);
	if (done == FALSE) {
		grac_log_error("grac_rule_udev_make_rules() : invalid map file");
		return FALSE;
	}

	done = _make_udev_rule_file(udev_rule, grac_rule, udev_rule_path);
	if (done == FALSE) {
		grac_log_error("grac_rule_udev_make_rules() : can't make udev rules file\n");
		return FALSE;
	}

	return done;
}

gboolean grac_rule_udev_make_empty(GracRuleUdev *udev_rule, const char *udev_rule_path)
{
	FILE	*out_fp;

	if (udev_rule == NULL || udev_rule_path == NULL)
		return FALSE;

	out_fp = g_fopen(udev_rule_path, "w");
	if (out_fp == NULL) {
		return FALSE;
	}

	_out_udev_rule_file_header(out_fp);

	fclose(out_fp);

	return TRUE;
}
