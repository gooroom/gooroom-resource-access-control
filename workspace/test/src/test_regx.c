/*
 * test_regx.c
 *
 *  Created on: 2017. 7. 13.
 *      Author: yang
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <glib.h>

#include <regex.h>


static char *pattern[] = {
		"^test$",
		"test",
		"test*",
		"*test*",
		"*.naver*",
		"naver.*",
		NULL
 };

static int find_match(char *str)
{
	int idx = -1;
	int	i;
	int res;
	regex_t	reg;

	for (i=0; ;i++) {
		if (pattern[i] == NULL)
			break;

		res = regcomp(&reg, pattern[i], REG_NOSUB);
		if (res) {
			printf("error : regcomp() - %s\n", strerror(errno));
			continue;
		}
		res = regexec(&reg, str, 0, NULL, 0);
		if (res == 0) {
			idx = i;
			break;
		}

		regfree(&reg);
	}

	return idx;
}

void test_regx()
{
	char	buf[256];
	int	  n, idx;

	while (1) {
		fgets(buf, sizeof(buf), stdin);
		n = strlen(buf);
		if (n > 0 && buf[n-1] < 0x020) {
			buf[n-1] = 0;
			n--;
		}
		if (n <= 0)
			break;
		idx = find_match(buf);
		if (idx < 0)
			printf("Can't match : [%s]\n", buf);
		else
			printf("Yes found : %d [%s] -> [%s]\n", idx, buf, pattern[idx]);
	}
}
