#include "utils/json_utils.h"

#include <stdlib.h>
#include <string.h>

int json_try_get_int(const cJSON* obj, const char* key, int* out) {
    const cJSON* item = NULL;

    if (obj == NULL || key == NULL || out == NULL) {
        return 0;
    }

    item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsNumber(item)) {
        return 0;
    }

    *out = (int)cJSON_GetNumberValue(item);
    return 1;
}

int json_try_get_long(const cJSON* obj, const char* key, long* out) {
    const cJSON* item = NULL;

    if (obj == NULL || key == NULL || out == NULL) {
        return 0;
    }

    item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsNumber(item)) {
        return 0;
    }

    *out = (long)cJSON_GetNumberValue(item);
    return 1;
}

int json_try_get_string(const cJSON* obj, const char* key, char** out) {
    const cJSON* item = NULL;
    size_t len;
    char* copy = NULL;

    if (obj == NULL || key == NULL || out == NULL) {
        return 0;
    }

    item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        return 0;
    }

    len = strlen(item->valuestring);
    copy = (char*)malloc(len + 1);
    if (copy == NULL) {
        return 0;
    }

    memcpy(copy, item->valuestring, len + 1);
    *out = copy;
    return 1;
}
// 把指定字段转换成JSON字符串输出，包含括号/换行
// 返回值需要用 cJSON_free 释放
char* json_dup_string(const cJSON* obj, const char* key) {
    const cJSON* item = NULL;

    if (obj == NULL || key == NULL) {
        return NULL;
    }

    item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (item == NULL) {
        return NULL;
    }

    return cJSON_Print(item);
}
