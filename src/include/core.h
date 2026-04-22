#pragma once

#include <curl/curl.h>

typedef struct {
    char* chunk;
    size_t size;
} Data;


typedef enum {
    STATUS_OK = 0,
    STATUS_CURL_ERROR,
    STATUS_HTTP_ERROR,
    STATUS_INTERNAL_ERROR
    
} StatusCode;

typedef struct {
    long http_code;
    long redirect_count;
    double total_time;
    curl_off_t size_download;
} Response;

typedef struct {
    StatusCode status;
    CURLcode curl_code;
    const char* msg;
    char curl_error[CURL_ERROR_SIZE];
    Response resp;
} Status;

typedef struct {
    Data MatchListData;
} CoreData;