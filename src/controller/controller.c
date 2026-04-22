// 负责整合各个模块，由main.c调用并传入必要数据，之后的一切让controller.c调度
#include <stdio.h>
#include <curl/curl.h>
#include "utils/logger.h"
#include "crawler/crawler.h"
// 一个任务一个函数，一个函数一个status
void check(){}
CoreData coredata;
void startApp(char* username){
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if(logger_init) {
        fprintf(stderr, "日志文件初始化失败，使用控制台输出\n");
    }
   log_message(INFO,"开始执行任务...");

   Status MatchListStatus;
   coredata.MatchListData =  getMatchList(&MatchListStatus);
   if(MatchListStatus.status == STATUS_OK) {
    log_message(INFO,"获取全部比赛列表执行完毕。");
   }
   else {
        LogLevel level = coredata.MatchListData.size==0?ERROR:WARNING;
        log_message(level,"获取全部比赛列表未正常执行");
        detailed_log(&MatchListStatus);
   }
}

