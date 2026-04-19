#include<curl/curl.h>
#include<stdlib.h>
#include "utils/logger.h"
#include "crawler/crawler.h"

static CURL *global_curl = NULL; 

size_t write_callback(char* data,size_t size,size_t nmemb,void* userp){
    size_t realsize = size * nmemb;
    Data *chunk = (Data*)userp;

    char msg[1000]='write_callback working';
    sprintf(msg,"[write_callback]:获取到%zu字节的数据",realsize);
    log_message(INFO,msg);

    char* new_chunk_ptr = realloc(chunk->chunk,chunk->size+realsize+1);
    if(new_chunk_ptr==NULL){
        sprintf(msg,"[write_callback]:爬取数据时分配内存失败！");
        log_message(ERROR,msg);
        return 0;
    }
    chunk->chunk = new_chunk_ptr;
    memcpy(&(chunk->chunk[chunk->size]),data,realsize);
    chunk->size = chunk->size + realsize;
    chunk->chunk[chunk->size] = 0;

    return realsize;
}
void set_custom_options(CURL *curl) {
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 设置超时时间为10秒
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // 允许重定向
}

// 获取比赛列表，注意并不全是用户参加的比赛。
Data getMatchList(Data data) {
    CURL *curl = global_curl;
    CURLcode code;
    if(curl == NULL) {
        log_message(WARNING, "全局curl为NULL，尝试在本函数初始化curl...");
        curl = curl_easy_init();
        if(curl == NULL){
            log_message(ERROR, "函数内初始化curl也失败，程序即将退出....");
            return;
        }
        else {
            log_message(INFO, "函数内初始化curl成功");
            global_curl = curl; 
            if(global_curl == NULL) {
                log_message(ERROR, "将函数内初始化的curl赋值给全局变量失败，为防止内存泄露，已停止运行！");
                return;
            }
        }
    }
    log_message(INFO,"正在执行getMatchList...");
    curl_easy_reset(curl);
    set_custom_options(curl);
    curl_easy_setopt(curl, CURLOPT_URL, BASE_URL "/contest.list");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void*)&data);
    code = curl_easy_perform(curl);
    if(code != CURLE_OK) {
        log_message(ERROR,"getMatchList出错！");
    }
    // 但是我怎么检查这个data的合法性呢，万一这个连接是中途歇逼的呢...?
    return data;
}
