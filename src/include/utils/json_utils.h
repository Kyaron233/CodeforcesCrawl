#pragma once

#include "cJSON.h"

int json_try_get_int(const cJSON* obj, const char* key, int* out);
int json_try_get_long(const cJSON* obj, const char* key, long* out);
char* json_dup_string(const cJSON* obj, const char* key);
