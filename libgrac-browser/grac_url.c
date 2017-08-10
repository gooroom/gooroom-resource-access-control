/*
 ============================================================================
 Name        : grac_url.c
 Author      : 
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */


#include "grm_log.h"
#include "grac_url.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <regex.h>

// Notice!  브라우저용 URL 목록이므로 주소(숫자)를 사용할 일이 없다고 가정
// only single URL (regular expression)

struct _GracUrl
{
	char	*pattern;
};


GracUrl* grac_url_alloc()
{
	GracUrl *url = malloc(sizeof(GracUrl));
	if (url) {
		memset(url, 0, sizeof(GracUrl));
	}

	return url;
}

GracUrl* grac_url_alloc_copy(GracUrl *org_url)
{
	if (org_url == NULL)
		return NULL;

	GracUrl *new_url = grac_url_alloc();
	if (new_url) {
		gboolean res = TRUE;

		if (org_url->pattern) {
			new_url->pattern = strdup(org_url->pattern);
			if (new_url->pattern == NULL)
				res = FALSE;
		}

		if (res == FALSE) {
			grac_url_free(&new_url);
		}
	}

	return new_url;
}

void    grac_url_init(GracUrl *url)
{
	if (url) {
		if (url->pattern != NULL)
			free(url->pattern);
		memset(url, 0, sizeof(GracUrl));
	}
}



void grac_url_free(GracUrl **purl)
{
	if (purl != NULL && *purl != NULL) {
		GracUrl *url = *purl;

		grac_url_init(url);

		free(url);

		*purl = NULL;
	}
}

gboolean grac_url_set_pattern(GracUrl *url, char *pattern)
{
	gboolean ret = FALSE;

	if (url && pattern) {
		char *ptr = strdup(pattern);
		if (ptr) {
			if (url->pattern)
				free(url->pattern);
			url->pattern = ptr;
			ret = TRUE;
		}
	}

	return ret;
}

char*  grac_url_get_pattern(GracUrl *url)
{
	char *pat = NULL;

	if (url)
		pat = url->pattern;

	return pat;
}


gboolean grac_url_is_match(GracUrl *url, char *url_str)
{
	gboolean ret = FALSE;

	if (url && url_str) {
		if (url->pattern) {
			regex_t	reg;
			int res = regcomp(&reg, url->pattern, REG_NOSUB);
			if (res) {
				grm_log_error("grac_url_is_match : regcomp() : [%s][%s] : %s",
											url, url->pattern, url, strerror(errno));
			}
			else {
				res = regexec(&reg, url_str, 0, NULL, 0);
				if (res == 0) {
					//grm_log_debug("matched : [%s] [%s], url->pattern, url_str);
					ret = TRUE;
				}
				else {
					//grm_log_debug("not matched : [%s] [%s]", url->pattern, url_str);
				}
			}
			regfree(&reg);
		}
	}

	return ret;
}

