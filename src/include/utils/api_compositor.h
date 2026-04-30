#pragma once

#include <stddef.h>

char* generate_url_with_access_token(const char* key,
                                     const char* secret,
                                     const char* method,
                                     size_t param_count,
                                     const char* const* kv_pairs);
