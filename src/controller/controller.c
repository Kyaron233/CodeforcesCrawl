// 负责整合各个模块，由main.c调用并传入必要数据，之后的一切让controller.c调度
#include <stdio.h>
#include <curl/curl.h>
#include "utils/logger.h"
#include "crawler/crawler.h"
#define CURL_QUENE QueneCount  // 这里是我们需要使用curl请求数据的次数
// 一个任务一个函数，一个函数一个status

// 这里用static有问题吗
static Data coredata[CURL_QUENE];
static Status corestatus[CURL_QUENE];


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
        detailed_log(&corestatus[task_order]);
    }
}

void startApp(char* username){
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if(logger_init()) {
        fprintf(stderr, "日志文件初始化失败，使用控制台输出\n");
    }
   log_message(INFO,"开始执行任务...");
   

   // 写给自己：为什么不把两个函数封装到一个逻辑呢？
   // 因为我不知道需要哪些参数，导致签名不同，这玩意又不能像go一样传指针这么方便
   // 至少我现在没想到解决方案好吧 （0422，17:38）
   coredata[ContestListData] =  getContestList(&corestatus[ContestListData]);
   check(ContestListData);

   coredata[UserRatingData] = getUserAttendedContestList(&corestatus[UserRatingData],username);
   check(UserRatingData);

   coredata[UserStatusData] = getUserStatus(&corestatus[UserStatusData],username);
   check(UserStatusData); 

    coredata[UserInfoData] = getUserInfo(&corestatus[UserInfoData],username);
    check(UserInfoData); 
   
}

