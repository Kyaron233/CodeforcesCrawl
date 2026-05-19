// 负责整合各个模块，由main.c调用并传入必要数据，之后的一切让controller.c调度
#include <stdio.h>
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

void check(quene task_order){
    const char* taskname = get_task_name(task_order);
    char message[128];

    if(corestatus[task_order].status == STATUS_OK) {
        snprintf(message, sizeof(message), "%s执行完毕。", taskname);
        log_message(INFO, message);
    }
    else {
        LogLevel level = coredata[task_order].size == 0 ? ERROR : WARNING;
        snprintf(message, sizeof(message), "%s未正常执行", taskname);
        log_message(level, message);
    }

    detailed_log(&corestatus[task_order]);
}

void crawl_data(char* username){
    coredata[ContestListData] =  getContestList(&corestatus[ContestListData]);
    check(ContestListData);

    coredata[UserRatingData] = getUserAttendedContestList(&corestatus[UserRatingData],username);
    check(UserRatingData);

    coredata[UserStatusData] = getUserStatus(&corestatus[UserStatusData],username);
    check(UserStatusData);

    coredata[UserInfoData] = getUserInfo(&corestatus[UserInfoData],username);
    check(UserInfoData);
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
        fprintf(stderr, "日志文件初始化失败，使用控制台输出\n");
    }
   log_message(INFO,"开始执行任务...");
   
   crawl_data(username);
   parse_and_output(coredata,username);
   append_user_list(username);
   
}

