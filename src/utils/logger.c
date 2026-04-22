#include<stdio.h>
#include<string.h>
#include "utils/logger.h"
#include <stdlib.h>


static FILE* global_logger = NULL; // 全局的日志文件句柄
// ...傻X依赖注入 

int logger_init() {
    
    FILE* logger = fopen("log/Log.txt", "a"); // 如果不存在就会创建一个,不需要自己创
    if(logger == NULL) {
        fprintf(stderr, "无法打开日志文件!，请检查文件权限\n");
        return 0; 
    }
    global_logger = logger; 
    return 1; 
}

void log_message(LogLevel level, const char* message) {
    FILE* logger = global_logger; 
    const char* level_str;
    switch(level) {
        case INFO:
            level_str = "INFO";
            break;
        case WARNING:
            level_str = "WARNING";
            break;
        case ERROR:
            level_str = "ERROR";
            break;
        default:
            level_str = "UNKNOWN";
    }
    // 输出到文件
    if(logger != NULL) fprintf(logger, "[%s] %s\n", level_str, message);
    
    // 输出到控制台
    switch(level) {
        case INFO:
            LOG_INFO("%s", message);
            break;
        case WARNING:
            LOG_WARN("%s", message);
            break;
        case ERROR:
            LOG_ERROR("%s", message);
            break;
        default:
            printf("[UNKNOWN] %s\n", message);
    }
}