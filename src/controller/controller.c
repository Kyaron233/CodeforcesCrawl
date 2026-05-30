// 负责整合各个模块，由main.c调用并传入必要数据，之后的一切让controller.c调度
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "utils/logger.h"
#include "cJSON.h"
#include "utils/output.h"
#include "crawler/crawler.h"
#define QUENE_LEN QueneCount  // 这里是我们需要使用curl请求数据的次数
// 一个任务一个函数，一个函数一个status

// 这里用static有问题吗
// 没有问题，static会让这里的数据的生命周期跟整个文件一致，并不是不让改
static Data coredata[QUENE_LEN];
static Status corestatus[QUENE_LEN];

int validUsername(char* username){
    // 检测用户名是否合法，只允许英文26字符和数字
    if (username == NULL || username[0] == '\0') {
        return 0;
    }
    for (const unsigned char* p = (const unsigned char*)username; *p != '\0'; ++p) {
        if (!((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||(*p != '_')||(*p != '-'))) {
            return 0;
        }
    }
    return 1;
}

static const char* get_task_name(quene task_order) {
    switch (task_order) {
        case ContestListData:
            return "获取全部比赛列表";
        case UserRatingData:
            return "获取用户参赛列表";
        case UserStatusData:
            return "获取用户提交列表";
        case UserInfoData:
            return "获取用户基本信息";
        default:
            return "未知任务";
    }
}

int check(quene task_order){
    const char* taskname = get_task_name(task_order);
    char message[128];

    if(corestatus[task_order].status == STATUS_OK) {
        snprintf(message, sizeof(message), "%s执行完毕。", taskname);
        log_message(LOG_INFO, message);
        //  detailed_log(&corestatus[task_order]);
        return 1;
    }
    else {
        LogLevel level = coredata[task_order].size == 0 ? LOG_ERROR : LOG_WARNING; // 除非请求根本没有发出，否则size好像无法等于0
        snprintf(message, sizeof(message), "%s未正常执行", taskname);
        log_message(level, message);
        detailed_log(&corestatus[task_order]);
        return 0;
    }

    detailed_log(&corestatus[task_order]);
}

int crawl_data(char* username){
    coredata[ContestListData] =  getContestList(&corestatus[ContestListData]);
    coredata[UserRatingData] = getUserAttendedContestList(&corestatus[UserRatingData],username);
    coredata[UserStatusData] = getUserStatus(&corestatus[UserStatusData],username);
    coredata[UserInfoData] = getUserInfo(&corestatus[UserInfoData],username);
    

    for(int  curTask=0;curTask<QueneCount;curTask++){
        if(!check(curTask)) {
            log_message(LOG_ERROR,"获取数据失败！可能是因为用户不存在或者网络错误，停止执行。");

            return 0;
        }
    }
    return 1;
}

void parse_data(Data* coredata){
    cJSON* obj_tree[QUENE_LEN];
    for(int i=0;i<QUENE_LEN;i++) {
        //注意chunk才是字符串实际存在的地方
        obj_tree[i]=cJSON_Parse(coredata[i].chunk);
    }
    
    
}

void startApp(char* username){
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if(!logger_init()) {
        // fprintf(stderr, "日志文件初始化失败，使用控制台输出\n");
    }
   log_message(LOG_INFO,"开始执行任务...");
   
   if(crawl_data(username)) 
        log_message(LOG_INFO,"数据爬取完成，进入下一步。。。");
   
   else {
        log_message(LOG_ERROR,"爬取数据失败！程序关闭中..");
        return 0;
   }
   parse_and_output(coredata,username);
   append_user_list(username);
}

void startAppMulti(char* listname){
    FILE* list = NULL;
    char* buffer = NULL;
    long size = 0;
    cJSON* userTree = NULL;
    char message[256];
    int executed = 0;

    if (listname == NULL || listname[0] == '\0') {
        log_message(LOG_ERROR, "列表文件名为空");
        return;
    }

    list = fopen(listname, "rb");
    if (list == NULL) {
        snprintf(message, sizeof(message), "无法打开文件: %s", listname);
        log_message(LOG_ERROR, message);
        return;
    }

    if (fseek(list, 0, SEEK_END) != 0) {
        log_message(LOG_ERROR, "读取文件大小失败");
        fclose(list);
        return;
    }

    size = ftell(list);
    if (size < 0) {
        log_message(LOG_ERROR, "读取文件大小失败");
        fclose(list);
        return;
    }

    rewind(list);
    if (size == 0) {
        log_message(LOG_WARNING, "列表文件为空");
        fclose(list);
        return;
    }

    buffer = (char*)malloc((size_t)size + 1);
    if (buffer == NULL) {
        log_message(LOG_ERROR, "内存分配失败");
        fclose(list);
        return;
    }

    if (fread(buffer, 1, (size_t)size, list) != (size_t)size) {
        log_message(LOG_ERROR, "读取文件失败");
        fclose(list);
        free(buffer);
        return;
    }
    buffer[size] = '\0';
    fclose(list);

    userTree = cJSON_Parse(buffer);
    if (userTree != NULL && cJSON_IsArray(userTree)) {
        int total = cJSON_GetArraySize(userTree);
        cJSON* item = NULL;

        log_message(LOG_INFO, "读取到json列表，处理中...");
        snprintf(message, sizeof(message), "json列表包含%d个条目", total);
        log_message(LOG_INFO, message);

        cJSON_ArrayForEach(item, userTree) {
            const char* username = NULL;

            if (cJSON_IsString(item)) {
                username = item->valuestring;
            } else if (cJSON_IsObject(item)) {
                cJSON* name = cJSON_GetObjectItemCaseSensitive(item, "username");
                if (!cJSON_IsString(name)) {
                    name = cJSON_GetObjectItemCaseSensitive(item, "handle");
                }
                if (cJSON_IsString(name)) {
                    username = name->valuestring;
                }
            }

            if (username == NULL || username[0] == '\0') {
                log_message(LOG_WARNING, "json列表中存在无效条目，已跳过");
                continue;
            }

            if (!validUsername((char*)username)) {
                snprintf(message, sizeof(message), "用户名非法，已跳过: %s", username);
                log_message(LOG_WARNING, message);
                continue;
            }

            snprintf(message, sizeof(message), "开始处理用户: %s", username);
            log_message(LOG_INFO, message);
            startApp((char*)username);
            executed++;
        }

        snprintf(message, sizeof(message), "本次共执行%d个用户任务", executed);
        log_message(LOG_INFO, message);

        cJSON_Delete(userTree);
        free(buffer);
        return;
    }

    if (userTree != NULL) {
        log_message(LOG_WARNING, "json格式不是数组，按行读取列表");
        cJSON_Delete(userTree);
    } else {
        log_message(LOG_WARNING, "json解析失败，按行读取列表");
    }

    {
        int line_count = 0;
        for (long i = 0; i < size; ++i) {
            if (buffer[i] == '\n') {
                line_count++;
            }
        }
        if (size > 0 && buffer[size - 1] != '\n') {
            line_count++;
        }
        snprintf(message, sizeof(message), "列表共%d行，按行读取", line_count);
        log_message(LOG_INFO, message);
    }

    {
        char* cursor = buffer;
        while (*cursor != '\0') {
            char* line_start = cursor;
            char* line_end = cursor;

            while (*line_end != '\0' && *line_end != '\n' && *line_end != '\r') {
                line_end++;
            }

            if (*line_end != '\0') {
                *line_end = '\0';
                line_end++;
                if (*line_end == '\n' || *line_end == '\r') {
                    line_end++;
                }
            }
            cursor = line_end;

            while (*line_start == ' ' || *line_start == '\t') {
                line_start++;
            }

            {
                char* tail = line_start + strlen(line_start);
                while (tail > line_start && (tail[-1] == ' ' || tail[-1] == '\t')) {
                    tail--;
                }
                *tail = '\0';
            }

            if (line_start[0] == '\0') {
                log_message(LOG_WARNING, "发现空行，已跳过");
                continue;
            }

            if (!validUsername(line_start)) {
                snprintf(message, sizeof(message), "用户名非法，已跳过: %s", line_start);
                log_message(LOG_WARNING, message);
                continue;
            }

            snprintf(message, sizeof(message), "开始处理用户: %s", line_start);
            log_message(LOG_INFO, message);
            startApp(line_start);
            executed++;
        }
    }

    snprintf(message, sizeof(message), "本次共执行%d个用户任务", executed);
    log_message(LOG_INFO, message);
    free(buffer);
}

