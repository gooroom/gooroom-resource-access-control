/*
 * grac_map.c
 *
 *  Created on: 2016. 6. 30.
 *      Author: yang
 */

#include "grac_map.h"

/**
  @file 	 	grac_map.c
  @brief		key 와 data로 구성되는 데이터 관리
  @details	class GracMap 구현 (GTK style의 c언어를 이용한 객체처리 사용) \n
  				key, data는 모두 문자열로 처리한다. \n
  				헤더파일 :  	grac_map.h	\n
  				라이브러리:	libgrac.so
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>

struct _GracMap {
	GHashTable *table;
	GHashTableIter iter;
};


static gpointer str_allocate(const char* data)
{
	char *ptr = NULL;

	if (data) {
		ptr = strdup(data);
	}

	return ptr;
}

static void str_destroy(gpointer p)
{
	if (p)
		free(p);
}


/**
 @brief   GracMap 객체 생성
 @details
      객체 지향형 언어의 생성자에 해당한다.
 @return 생성된 객체 주소
 */
GracMap* grac_map_alloc()
{
	GracMap *map = malloc(sizeof(GracMap));
	if (map) {
		memset(map, 0, sizeof(GracMap));
		map->table = g_hash_table_new_full(g_str_hash, g_str_equal, str_destroy, str_destroy);
		if (map->table == NULL) {
			free(map);
			map = NULL;
		}
	}

	return map;

}

/**
@brief   GracMap 객체 해제
@details
      객체 지향형 언어의 소멸자에  해당한다. 단 자동으로 불려지지 않고 필요한 시점에서 직접 호출하여야 한다.\n
      해제된  객체 주소값으로 0으로 설정하기 위해 이중 포인터를 사용한다.
@param [in,out]  pmap  해제하고자 하는 객체 주소를 담는 변수의 주소 (double pointer)
 */
void	 grac_map_free(GracMap **pmap)
{
	if (pmap != NULL && *pmap != NULL) {
		GracMap *map = *pmap;

		if (map->table != NULL) {
			g_hash_table_remove_all(map->table);
			g_hash_table_unref(map->table);
		}

		free(map);

		*pmap = NULL;
	}
}


void grac_map_clear(GracMap* map)
{
	if (map != NULL) {
		if (map->table != NULL) {
			g_hash_table_remove_all(map->table);
		}
	}
}

/**
  @brief   주어진 키의 데이터를 설정한다.
  @details
      키가 없는 경우는 새로이 생성되고 키가 이미 있는 경우는 데이터가 치환된다.
  @param [in]  map  GracMap 객체주소
  @param [in]  key    키 값
  @param [in]  data  설정할 데이터
  @return gboolean 성공여부
 */
gboolean grac_map_set_data(GracMap *map, const char *key, const char *data)
{
	gboolean ret = FALSE;

	if (map && key && data) {
		gpointer new_key = str_allocate(key);
		gpointer new_data = str_allocate(data);
		if (new_key && new_data) {
			g_hash_table_insert(map->table, new_key, new_data);  // if key is existed, change value
			ret = TRUE;
		}
		else {
			if (new_key)
				str_destroy(new_key);
			if (new_data)
				str_destroy(new_data);
		}
	}

	return ret;
}

/**
  @brief   주어진 키의 데이터를 삭제한다.
  @details
      데이터와 더불어 키도 삭제된다.
  @param [in]  map  GracMap 객체주소
  @param [in]  key    삭제할 데이터의 키 값
  @return gboolean 성공여부
 */
gboolean grac_map_del_data(GracMap *map, const char *key)
{
	gboolean ret = FALSE;

	if (map && key)
		ret = g_hash_table_remove(map->table, key);

	return ret;
}

/**
  @brief   주어진 키의 데이터를 한다.
  @details
      등록되지 않은 키인 경우는 NULL을 리턴한다.
  @param [in]  map  GracMap 객체주소
  @param [in]  key    찾고자 하는 데이터의 키 값
  @return const char*	데이터값,  오류 또는 없는 경우는 NULL
 */
const char* grac_map_get_data(GracMap *map, const char *key)
{
	const char *data = NULL;
	if (map && key) {
		gpointer p_data;
		p_data = g_hash_table_lookup(map->table, key);
		if (p_data)
			data = p_data;
	}

	return data;
}

/**
  @brief   주어진 키의 데이터의 길이를 한다.
  @details
      등록되지 않은 키인 경우는 0 리턴한다.  길이는 문자열의 종료문자 0도 포함한다.
  @param [in]  map  GracMap 객체주소
  @param [in]  key    데이터의 키 값
  @return int		데이터 길이(문자열의 종료문자도 포함),  오류 또는 없는 경우는 0
 */
int  grac_map_get_data_len(GracMap *map, const char *key)
{
	int	len = 0;
	const char* data;

	data = grac_map_get_data(map, key);
	if (data)
		len = strlen(data)+1;

	return len;
}

/**
  @brief   등록된 키를 모두 나열하고자 할 때 최초로 사용되는 함수이다.
  @details
  	  이 후 grac_map_next_key()를 반복 호출하여 키를 구한다.	\n
      등록된 키가 하나도 없는 경우 NULL을 리턴한다.
  @param [in]  map  GracMap 객체주소
  @return const char*	키값,  오류 또는 더 이상 없는 경우 NULL
 */
const char* grac_map_first_key(GracMap *map)
{
	const char *key = NULL;
	if (map) {
		g_hash_table_iter_init(&map->iter, map->table);
		key = grac_map_next_key (map);
	}

	return key;
}

/**
  @brief   등록된 키가 더 있는지 확인한다.
  @details
  	  키 나열을 위해서는 처음에는 grac_map_first_key()를 사용하여 키를 얻은 후 \n
  	  grac_map_next_key()를 반복 호출하여 키를 구한다.	\n
      등록된 키가 더 이상 없는 경우는  NULL을 리턴한다.
  @param [in]  map  GracMap 객체주소
  @return const char*	키값,  오류 또는 더 이상 없는 경우 NULL
 */
const char* grac_map_next_key (GracMap *map)
{
	const char *key = NULL;
	if (map) {
		gpointer p_key;
		gpointer p_value;
		if (g_hash_table_iter_next(&map->iter, &p_key, &p_value)) {
			key = p_key;
		}
	}

	return key;
}

