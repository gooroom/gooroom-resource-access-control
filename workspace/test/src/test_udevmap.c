/*
 * test_udevmap.c
 *
 *  Created on: 2017. 9. 4.
 *      Author: yang
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "grac_rule.h"
#include "grac_resource.h"
#include "cutility.h"



// # [grac-rule] resource permission
// # comment
// rule1
// rule2
// #rule3
// empty line

#define GRAC_RULE_MAP_KEY	"[grac-rule]"

gboolean	parse_rule_line(char *buf, int buf_size, int *res_id, int *perm_id, char *opt, int opt_size)
{
	gboolean done = FALSE;

	if (buf && res_id && perm_id && opt) {
		char word[256];
		char *ptr;
		int len;

		ptr = c_stristr(buf, GRAC_RULE_MAP_KEY, buf_size);
		if (ptr) {
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
			}
			else {
				done = FALSE;
			}
			ptr += len;

			// permisssion
			len = c_get_word(ptr, buf_size, NULL, word, sizeof(word));
			if (len > 0) {
				int id = grac_resource_get_permission_id(word);
				if (perm_id >= 0)
					*perm_id = id;
				else
					done = FALSE;
			}
			else {
				done = FALSE;
			}
			ptr += len;

			len = c_get_word(ptr, buf_size, NULL, word, sizeof(word));
			if (len > 0) {
				c_strcpy(opt, word, opt_size);
			}
			else {
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
//
// return -1:error 0:empty 1:normal 2:comment 3:map info

#define LINE_KIND_ERROR		-1
#define LINE_KIND_EMPTY		0
#define LINE_KIND_COMMENT	1
#define LINE_KIND_RULE		2
#define LINE_KIND_MAP			3

int	check_line(char *buf, int buf_size)
{
	int kind = LINE_KIND_ERROR;
	int	idx;

	if (buf) {

		kind = LINE_KIND_EMPTY;

		for (idx=0; idx<buf_size; idx++) {
			int ch;
			ch = buf[idx] & 0x0ff;
			if (ch == 0)
				break;
			if (ch > 0x20) {
				if (ch == '#') {
					char word[256];
					int	len = c_get_word(buf+1, buf_size-1, NULL, word, sizeof(word));
					if (len > 0 && c_strimatch(word, GRAC_RULE_MAP_KEY))
						kind = LINE_KIND_MAP;
					else
						kind = LINE_KIND_COMMENT;
				}
				else
					kind = LINE_KIND_RULE;
				break;
			}
		} // for

	}

	return kind;
}


gboolean check_rule(GracRule *rule, int res_id, int perm_id)
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

int	get_line_count(char *file)
{
	FILE 	*fp;
	char	buf[2048];
	int		count = 0;

	fp = fopen(file, "r");
	if (fp != NULL) {
		while (1) {
			if (fgets(buf, sizeof(buf), fp) == NULL)
				break;
			count++;
		}
		fclose(fp);
	}

	return count;
}

gboolean analyze_line_status(GracRule* rule, char *file, int *status, int status_count)
{
	FILE 	*fp;
	char	buf[1024];
	int		idx;
	int 	kind;
	int		rule_seq = 0;
	int 	map_on = 0;
	int		header_status = 0;	// 0: not start 1:collecting -1:end of haeder
	gboolean out_line;
	gboolean check = FALSE;

	fp = fopen(file, "r");
	if (fp == NULL)
		return FALSE;

	for (idx=0; idx<status_count; idx++) {
		if (fgets(buf, sizeof(buf), fp) == NULL)
				break;

		kind = check_line(buf, sizeof(buf));

		out_line = FALSE;

		if (header_status == 0) {
			if (kind == LINE_KIND_EMPTY) {
				out_line = TRUE;
			}
			else if (kind == LINE_KIND_COMMENT) {
				out_line = TRUE;
				header_status = 1;
			}
			else {
				header_status = -1;
			}
		}
		else if (header_status == 1) {
			if (kind == LINE_KIND_EMPTY) {
				out_line = TRUE;
				header_status = -1;
			}
			else if (kind == LINE_KIND_COMMENT) {
				out_line = TRUE;
			}
			else {
				header_status = -1;
			}
		}

		if (kind == LINE_KIND_MAP) {
			if (map_on == 0) {
				check = FALSE;
				rule_seq++;
			}
			map_on = 1;
		}

		if (map_on)
			status[idx] = rule_seq;
		else
			status[idx] = 0;
		if (kind == LINE_KIND_COMMENT)
			status[idx] |= 0x4000;

		if (kind == LINE_KIND_MAP) {
			int res_id, perm_id;
			char opt[256];
			gboolean	res;
			res = parse_rule_line(buf, sizeof(buf), &res_id, &perm_id, opt, sizeof(opt));
			if (res == FALSE)
				printf("---> invalid line\n");
			else
				printf("---> res:%d perm:%d opt:[%s]\n", res_id, perm_id, opt);
			if (res) {
				check |= check_rule(rule, res_id, perm_id);
				// adjust flag for previous lines
				if (check) {
					int prev;
					for (prev=idx-1; prev>=0; prev--) {
						int pseq = status[prev] & 0x3fff;
						if (pseq == rule_seq || (status[prev] & 0x4000))
							status[prev] |= 0x8000;
						else
							break;
					}
				}
			}
		}
		if (check && map_on)
			out_line = TRUE;

		// check and commnet & rule
		if (out_line)
			status[idx] |= 0x8000;

		if (kind == 0) {
			map_on = 0;
			check = FALSE;
		}

	}

	fclose(fp);

	return TRUE;
}

gboolean make_udev_rule_file(char *file, int *status, int status_count)
{
	FILE	*fp;
	char	buf[2048];

	fp = fopen(file, "r");
	if (fp == NULL) {
		return FALSE;
	}

	int idx;
	int	seq, out;

	for (idx=0; idx<status_count; idx++) {
		if (fgets(buf, sizeof(buf), fp) == NULL)
				break;

		seq = status[idx] & 0x3fff;
		if (status[idx] & 0x8000)
			out = 1;
		else
			out = 0;

		if (out)
			printf("*[%04x][%d][%d]-%s", status[idx], out, seq, buf);
		else
			printf(" [%04x][%d][%d]-%s", status[idx], out, seq, buf);

	}

	fclose(fp);

	return TRUE;
}


void test_udev_map(GracRule *rule)
{
	int		line_count;
	int		*line_status;
	char	*map_file_path = "test-rule.map";
	gboolean res;

	line_count = get_line_count(map_file_path);
	if (line_count <= 0) {
		printf("not founed or invalid map file\n");
		return;
	}

	line_status = malloc(line_count * sizeof(int));
	if (line_status == NULL) {
		printf("out of memory\n");
		return;
	}
	memset(line_status, 0, line_count * sizeof(int));

	res = analyze_line_status(rule, map_file_path, line_status, line_count);
	if (res == FALSE) {
		printf("invalid map file\n");
		return;
	}

	res = make_udev_rule_file(map_file_path, line_status, line_count);
	if (res == FALSE) {
		printf("can't make udev rules file\n");
		return;
	}
}

void t_udev_map()
{
	GracRule	*rule = grac_rule_alloc();

	if (grac_rule_load(rule, NULL) == FALSE) {
		printf("Load rule error\n");
	}

	test_udev_map(rule);

	grac_rule_free(&rule);

}


