/*
 * cutility.c
 *
 *  Created on: 2015. 11. 19.
 *      Author: user
 */

/**
  @file 	 	cutility.c
  @brief		C 언어용 라이브러리
  @details	특정 프로젝트에 독립적이고 표쥰 함수만을 이용해 구현되었다 \n
  				헤더파일 :  	cutility.h	\n
  				라이브러리:	libgrac.so
 */

#include "cutility.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

 /**
  @brief  문자열의 앞 부분과 뒷부분의 공백문자를 제거한다.
  @details
       공벡무자의 범위는 0x01 ~ 0x20 이다 \n
       주어진 문자열이 NULL인 경우는 무시된다.
  @param [in,out]		str	 공백문자를 제거할 문자열 (string)
  @param [in]		    size str의 버퍼 크기
*/
void c_strtrim(char *str, int size)
{
	c_strltrim(str, size);
	c_strrtrim(str, size);
}

/**
  @brief  문자열의 앞부분의 공백문자를 제거한다.
  @details
       공벡무자의 범위는 0x01 ~ 0x20 이다		\n
       주어진 문자열이 NULL인 경우는 무시된다.
  @param [in,out]		str	 공백문자를 제거할 문자열 (string)
  @param [in]		    size str의 버퍼 크기
*/
void c_strltrim(char *str, int size)
{
	if (str == NULL)
		return;

	unsigned char ch;
	int idx, n;
	n = c_strlen(str, size-1);
	for (idx=0; idx<n; idx++) {
		ch = str[idx];
		if (ch > 0x20)
			break;
	}
	if (idx > 0)
		c_strcpy(str, str+idx, size);
}

/**
  @brief  문자열의 뒷부분의 공백문자를 제거한다.
  @details
       공벡무자의 범위는 0x01 ~ 0x20 이다		\n
       주어진 문자열이 NULL인 경우는 무시된다.
  @param [in,out]		str	공백문자를 제거할 문자열 (string)
  @param [in]		    size str의 버퍼 크기
*/
void c_strrtrim(char *str, int size)
{
	if (str == NULL)
		return;

	int idx, n;
	unsigned char ch;
	n = c_strlen(str, size-1);
	for (idx=n-1; idx>=0; idx--) {
		ch = str[idx];
		if (ch > 0x20)
			break;
	}
	str[idx+1] = 0;
}

/**
  @brief  문자열의 길이를 구한다.
  @details
       표준함수 strlen()과 동일한 기능이나  문자열이 NULL인 경우는 길이를 0으로 처리한다..
  @param [in]	str	길이를 구할 문자열 (string)
  @param [in]	max 문자열의 최대 허용 길이
  @return int	문자열 길이
*/
int c_strlen(const char *str, int max)
{
	int	n = 0;

	if (str) {
		for (n=0; n < max; n++) {
			if (str[n] == 0)
				break;
		}
	}
	return n;
}

/**
  @brief   문자열을 복사한다.
  @details
       표준함수 strncpy()와 동일한 기능이나  문자열이 NULL인 경우는 아무처리도 하지 않는다
  @param [in,out]	dest		복사받을 버퍼
  @param [in]	src		복가할 문자열 (string)
  @param [in] size	dest의 버퍼 크기
*/
char* c_strcpy(char *dest, char *src, int size)
{
	if (dest && src) {
		strncpy(dest, src, size);
		dest[size-1] = 0;
	}

	return dest;
}

/**
  @brief  두 개의 문자열을 비교한다.
  @details
       표준함수 strcmp()와 동일한 기능이나  문자열이 NULL인 경우는 다음과 같이 처리한다 \n.
       두 문자열이 모두 NULL인 경우는  파라메터 both_nuul로 주어진 값을 돌려분다.
       하나가 NULL인 경우는 가장 적은 값으로 취급하여 1 또는 -1을 돌려준다 \n
       예)  c_strlen(NULL, "test"): ---> -1,      c_strlen("test", NULL): ---> 1
  @param [in]	s1		비교할 문자열 (string)
  @param [in]	s2		비교할 문자열 (string)
  @param [in] 	both_null	둘 다 NULL인 경우의 리턴값
  @return int	 비교 결과 : -1 (s1 < s2),   0 (s1 == s2),  1 (s1 > s2)
  @warning	NULL인 경우를 함수 호출전에 검토하여 별도로 처리하면 기존의 strcmp()와 동일하다.
*/
int  c_strcmp(const char *s1, const char *s2, int size, int both_null)
{
	if (s1 == NULL && s2 == NULL)
		return both_null;

	if (s1 == NULL)
		return -1;
	if (s2 == NULL)
		return 1;

	return strncmp(s1, s2, size);
}

/**
  @brief  문자열 버퍼를 생성하고 복사한다.
  @details
       표준함수 strdup()과 동일한 기능이나  다음과 같은 차이가 있다\n.
    src 문자열이 NULL인 경우는 결과도 NULL이다.
  @param [in]		src		복사에 사용될 문자열 (string)
  @warning	결과를 담는 버퍼의 원래 문자열은 항상 free되는 것에 주의하여야하고 따라서 초기값은 NULL로 설정되어 있어야한다.
  @return 새로 생성되어 복사된 문자열,  사용후에는 반드시 free() 한다.
*/
char* c_strdup(char *src, int max)
{
	char *dest = NULL;
	if (src)
		dest = strndup(src, max);

	return dest;
}

int   c_strmatch(const char *s1, const char *s2)
{
	if (s1 == NULL || s2 == NULL)
		return 0;

	int	i, n1, n2;

	n1 = c_strlen(s1, 65536);
	n2 = c_strlen(s2, 65536);
	if (n1 != n2 || n1 == 65536)
		return 0;

	for (i=0; i<n1; i++) {
		if (s1[i] != s2[i])
			return 0;
	}

	return 1;
}

int   c_strimatch(const char *s1, const char *s2)
{
	if (s1 == NULL || s2 == NULL)
		return 0;

	int	i, n1, n2;

	n1 = c_strlen(s1, 65536);
	n2 = c_strlen(s2, 65536);
	if (n1 != n2 || n1 == 65536)
		return 0;

	for (i=0; i<n1; i++) {
		int ch1 = toupper(s1[i]);
		int ch2 = toupper(s2[i]);
		if (ch1 != ch2)
			return 0;
	}

	return 1;
}


char*	c_strchr (char *src, int ch, int size)		// if not found, return NULL
{
	char *find = NULL;

	if (src) {
		int	i;
		int len = c_strlen(src, size-1);
		for (i=0; i<len; i++) {
			if (src[i] == ch) {
				find = src + i;
				break;
			}
		}
	}

	return find;
}

char*	c_strrchr(char *src, int ch, int size)		// if not found, return NULL
{
	char *find = NULL;

	if (src) {
		int	i;
		int len = c_strlen(src, size-1);
		for (i=len-1; i>=0; i--) {
			if (src[i] == ch) {
				find = src + i;
				break;
			}
		}
	}

	return find;
}

char*	c_strstr (char *src, char* str, int size)		// if not found, return NULL
{
	char *find = NULL;
	int len1, len2;

	len1 = c_strlen(src, size-1);
	len2 = c_strlen(str, size+1);

	if (len1 > 0 && len2 > 0) {
		int	i;

		for (i=0; i<= len1-len2; i++) {
			if (c_memcmp(src+i, str, len2, -1) == 0) {
				find = src + i;
				break;
			}
		}
	}
	return find;

}

char*	c_stristr (char *src, char* str, int size)		// if not found, return NULL
{
	char *find = NULL;
	int len1, len2;

	len1 = c_strlen(src, size-1);
	len2 = c_strlen(str, size+1);

	if (len1 > 0 && len2 > 0) {
		int	i;

		for (i=0; i<= len1-len2; i++) {
			if (c_memicmp(src+i, str, len2, -1) == 0) {
				find = src + i;
				break;
			}
		}
	}

	return find;

}

char*	c_strrstr(char *src, char* str, int size)		// if not found, return NULL
{
	char *find = NULL;
	int len1, len2;

	len1 = c_strlen(src, size-1);
	len2 = c_strlen(str, size+1);

	if (len1 > 0 && len2 > 0) {
		int	i;
		for (i=len1-len2; i>=0; i--) {
			if (c_memcmp(src+i, str, len2, -1) == 0) {
				find = src + i;
				break;
			}
		}
	}

	return find;
}

char*	c_strristr(char *src, char* str, int size)		// if not found, return NULL
{
	char *find = NULL;
	int len1, len2;

	len1 = c_strlen(src, size-1);
	len2 = c_strlen(str, size+1);

	if (len1 > 0 && len2 > 0) {
		int	i;
		for (i=len1-len2; i>=0; i--) {
			if (c_memicmp(src+i, str, len2, -1) == 0) {
				find = src + i;
				break;
			}
		}
	}

	return find;
}

void	c_strcat (char *dest, char* src, int size)
{
	if (dest == NULL || src == NULL)
		return;

	strncat(dest, src, size);
}

void 	c_memset(void *buf, int ch, int len)
{
	if (buf)
		memset(buf, ch, len);
}

void*	c_memcpy(void *dest, void* src,  int len)
{
	if (dest && src) {
		memmove(dest, src, len);
	}

	return dest;
}

int 	c_memcmp(void *buf1, void* buf2, int len, int ret_both_null)
{
	if (buf1 == NULL && buf2 == NULL)
		return ret_both_null;

	if (buf1 == NULL)
		return -1;
	if (buf2 == NULL)
		return 1;

	return memcmp(buf1, buf2, len);

}

int 	c_memicmp(void *buf1, void* buf2, int len, int ret_both_null)
{
	if (buf1 == NULL && buf2 == NULL)
		return ret_both_null;

	if (buf1 == NULL)
		return -1;
	if (buf2 == NULL)
		return 1;

	int i;
	char *b1 = buf1;
	char *b2 = buf2;
	for (i=0; i<len; i++) {
		int ch1 = toupper(b1[i]);
		int ch2 = toupper(b2[i]);
		if (ch1 < ch2)
			return -1;
		if (ch1 > ch2)
			return 1;
	}

	return 0;
}


/**
  @brief  calloc과 동일 기능
  @return char* 할당된 메모리 주소
*/
char* c_alloc(int size)
{
	char *mem = NULL;

	if (size > 0) {
		mem = malloc(size);
		if (mem)
			memset(mem, 0, size);
	}

	return mem;
}

/**
  @brief  malloc, calloc,realloc으로 할당한 메모리를 해제한다.
  @details
       표준함수 free()와 동일한 기능이나  NULL인 경우는 무시한다 \n.
       해제할 메모리는 이중 포인터로 넘겨 해제 후  메모리 주소를 NULL로 설정된다.
  @param [in,out]	ptr	해제할 메모리의 주소를 담고있는 변수의 주소
*/
void c_free(char **ptr)
{
	if (ptr) {
		if (*ptr)
			free(*ptr);
		*ptr = NULL;
	}
}

/**
  @brief  문자열을 숫자로 변환시킨다.
  @details
       표준함수 atoi()과 동일한 기능이나  다음과 같은 차이가 있다\n.
       number 값은 변환 오류시에는 -1을 돌려준다.\n
       10진수 변환을 하고 유효한 숫자까지만 처리하고 이후 어떤 문자가 오더라도 무시된다. \n
       단 10진수에 해당하는 숫자가 하나도 없는 경우는 오류로 처리한다..
  @param [in]		str		숫자를 포함하고 있는 문자열 (string)
  @param [in]		size	문자열 버퍼 크기
  @param [out]	value		변환결과, 오류시는 0, NULL인 경우 무시
  @return	int	변환에 사용된 버퍼 길이
*/
int	c_get_number(char *str, int size,  int* value)
{
	int n = 0;

	unsigned char ch;
	int cnt = 0;
	int idx = 0;
	int len;

	len = c_strlen(str, size-1);
	while (idx < len) {
		ch = *str;
		if (ch > 0x20)
			break;
		str++;
		idx++;
	}

	cnt = 0;
	n = 0;
	while (idx < len) {
		ch = *str;
		if (ch < '0' || ch > '9') {
			break;
		}
		n = n * 10 + (ch-'0');
		str++;
		cnt++;
		idx++;
	}

	if (cnt == 0)
		n = -1;

	if (value)
		*value = n;

	return idx;
}

/**
  @brief  문자열내에서 공백문자로 기준으로 문자열 또는 구분문자를 분리한다.
  @details
       주어진 문자열 처음부터 뒷부분으로 진행되며 아래 항목이 구해지면 결과를 돌려준다\n.
   	   공백문자와 delemeter로 구분되고  delemeter를 제외한 문자로 구성, \n
   	    또는  delmeter에 정의된 문자인 경우는 단일 문자로 구성
  @param [in]		buf		문자열을 분리한 소스 문자열 (string)
  @param [in]		bsize	버퍼 크기 (string 종료문자 0 포함)
  @param [in]		delemeter 단어구분 기호
  @param [out]	word	분리된 문자열을 담을 버퍼 주소
  @param [in]		wsize	결과를 담을 버퍼 크기 (string 종료문자 0 포함)
  @return	int	변환에 사용된 버퍼 길이
*/
int	c_get_word  (char *buf, int bsize, char* delemeter, char *word, int wsize)
{
	int idx, ch, len, cnt;

	if (bsize <= 1 || wsize <= 1)
		return 0;

	len = c_strlen(buf, bsize-1);
	for (idx=0; idx<len; idx++) {
		ch = buf[idx] & 0x0ff;
		if (ch > 0x20)
			break;
	}

	cnt = 0;
	for (; idx<len; idx++) {
		ch = buf[idx] & 0x0ff;
		if (ch <= 0x20)
			break;
		if (c_strchr(delemeter, ch, 1024)) {
			if (cnt == 0) {
				word[cnt] = (char)ch;
				cnt++;
				idx++;
			}
			break;
		}
		word[cnt] = (char)ch;
		cnt++;
		if (cnt >= wsize-1)
			break;
	}
	word[cnt] = 0;

	return idx;
}
