#include<stdio.h>
#include<string.h>
#include "utils/logger.h"
#include <stdlib.h>


static FILE* global_logger = NULL; // 全局的日志文件句柄
// ...傻X依赖注入 

int logger_init() {
    const char* candidates[] = {
        "log/log.txt",      // 从项目根目录运行
        "../log/log.txt",   // 从 build 目录运行
        "../../log/log.txt" // 从 build/linux 目录运行
    };

    FILE* logger = NULL;
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        logger = fopen(candidates[i], "a"); // 不存在会自动创建
        if (logger != NULL) {
            break;
        }
    }

    if(logger == NULL) {
        fprintf(stderr, "无法打开日志文件 log/log.txt（已尝试多个相对路径），请检查文件权限或运行目录\n");
        return 0; 
    }
    global_logger = logger; 
    return 1; 
}

void log_message(LogLevel level, const char* message) {
    FILE* logger = global_logger; 
    const char* level_str;
    switch(level) {
        case LOG_INFO:
            level_str = "LOG_INFO";
            break;
        case LOG_WARNING:
            level_str = "LOG_WARNING";
            break;
        case LOG_ERROR:
            level_str = "LOG_ERROR";
            break;
        default:
            level_str = "UNKNOWN";
    }
    // 输出到文件
    if(logger != NULL) fprintf(logger, "[%s] %s\n", level_str, message);
    
    // 输出到控制台
    switch(level) {
        case LOG_INFO:
            LOG_INFO("%s", message);
            break;
        case LOG_WARNING:
            LOG_WARN("%s", message);
            break;
        case LOG_ERROR:
            LOG_ERROR("%s", message);
            break;
        default:
            printf("[UNKNOWN] %s\n", message);
    }
}

void detailed_log(Status* status) {
    if (status == NULL) {
        log_message(LOG_ERROR, "Status is NULL");
        return;
    }

    const char* status_str = "UNKNOWN";
    switch (status->status) {
        case STATUS_OK:
            status_str = "STATUS_OK";
            break;
        case STATUS_CURL_ERROR:
            status_str = "STATUS_CURL_ERROR";
            break;
        case STATUS_HTTP_ERROR:
            status_str = "STATUS_HTTP_ERROR";
            break;
        case STATUS_INTERNAL_ERROR:
            status_str = "STATUS_INTERNAL_ERROR";
            break;
        default:
            break;
    }

    const char* msg = status->msg ? status->msg : "n/a";
    const char* curl_err = (status->curl_error[0] != '\0') ? status->curl_error : "n/a";
    char message[512];
    snprintf(
        message,
        sizeof(message),
        "Status=%s curl_code=%d http=%ld redirects=%ld total_time=%.3f size=%lld msg=%s curl_error=%s",
        status_str,
        (int)status->curl_code,
        status->resp.http_code,
        status->resp.redirect_count,
        status->resp.total_time,
        (long long)status->resp.size_download,
        msg,
        curl_err
    );

    LogLevel level = (status->status == STATUS_OK) ? LOG_INFO : LOG_WARNING;
    log_message(level, message);
}