#pragma once

#include "cJSON.h"

int json_try_get_int(const cJSON* obj, const char* key, int* out);
int json_try_get_long(const cJSON* obj, const char* key, long* out);
// Copies the raw string value; free with free().
int json_try_get_string(const cJSON* obj, const char* key, char** out);
// Returns JSON text for the field; free with cJSON_free.
char* json_dup_string(const cJSON* obj, const char* key);
