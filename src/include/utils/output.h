#pragma once

#include "cJSON.h"

int output_json(const cJSON* root, const char* filename);
int output_json_with_username(const cJSON* root, const char* username, const char* filename);
int output_rawstring_with_username(const char* raw, const char* username, const char* filename);
