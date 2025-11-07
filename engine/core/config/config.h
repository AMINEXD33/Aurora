#ifndef _CONFIG_HEADERS
#define _CONFIG_HEADERS

#include <cjson/cJSON.h>
#include "../../helpers/helpers.h"

cJSON *INIT_CORE_CONFIG();
int *GET_CORE_COUNT(cJSON *json);
int *GET_THREAD_COUNT(cJSON *json);
int *GET_MAX_THRESHOLD(cJSON *json);
int *GET_MIN_THRESHOLD(cJSON *json);
int *GET_THRESHOLD(cJSON *json);
int *GET_SHARED_MEMORY_UNITES(cJSON *json);





#endif