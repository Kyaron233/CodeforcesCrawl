#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/logger.h"
#include "crawler/crawler.h"
#include "core.h"

static CURL *global_curl = NULL; 

// 进阶版的日志
static void init_status(Status* status) {
    if (status == NULL) {
        return;
    }
    status->status = STATUS_INTERNAL_ERROR;
    status->curl_code = CURLE_OK;
    status->msg = "请求尚未执行";
    status->curl_error[0] = '\0';
    status->resp = (Response){0};
}

static void fill_response_meta(CURL* curl, Response* resp) {
    if (curl == NULL || resp == NULL) {
        return;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->http_code);
    curl_easy_getinfo(curl, CURLINFO_REDIRECT_COUNT, &resp->redirect_count);
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &resp->total_time);
    curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &resp->size_download);
}

size_t write_callback(char* data,size_t size,size_t nmemb,void* userp){
    size_t realsize = size * nmemb;
    Data *chunk = (Data*)userp;

    char msg[1000] = "write_callback working";

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
CURL* getCurl(){
    if(global_curl!=NULL) return global_curl;
    else{
        log_message(INFO,"全局curl句柄失效，正在尝试初始化");
        global_curl = curl_easy_init();
        if(global_curl==NULL) {
            log_message(ERROR,"无法初始化curl！");
            return NULL;
        }
        return global_curl;
    }
}
// “2.	抓取比赛列表和用户参加比赛的列表” 
// 获取比赛列表，注意并不全是用户参加的比赛。
Data getMatchList(Status* status) {
    Data data = {0};
    init_status(status);

    CURL *curl = getCurl();
    if(curl==NULL) {
        if (status != NULL) {
            status->status = STATUS_INTERNAL_ERROR;
            status->curl_code = CURLE_FAILED_INIT;
            status->msg = "curl句柄初始化失败";
        }
        return data;
    }

    CURLcode code;
    log_message(INFO,"正在执行getMatchList...");
    curl_easy_reset(curl);
    set_custom_options(curl);
    curl_easy_setopt(curl, CURLOPT_URL, BASE_URL "/contest.list");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void*)&data);
    if (status != NULL) {
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, status->curl_error);
    }

    code = curl_easy_perform(curl);

    if(code != CURLE_OK) {
        char log_buffer[512];
        const char* detail = curl_easy_strerror(code);

        if (status != NULL) {
            status->status = STATUS_CURL_ERROR;
            status->curl_code = code;
            fill_response_meta(curl, &status->resp);
            status->msg = (status->curl_error[0] != '\0') ? status->curl_error : curl_easy_strerror(code);
            detail = status->msg;
        }

        // snprintf(log_buffer, sizeof(log_buffer), "getMatchList出错：curl_code=%d, detail=%s", (int)code, detail);
        // log_message(ERROR, log_buffer);
    }
    else {
        if (status != NULL) {
            fill_response_meta(curl, &status->resp);
            status->curl_code = code;

            if (status->resp.http_code >= 200 && status->resp.http_code < 300 && data.chunk != NULL && data.size > 0) {
                status->status = STATUS_OK;
                status->msg = "请求成功";
            } else {
                status->status = STATUS_HTTP_ERROR;
                status->msg = "HTTP状态异常或响应体为空";
            }
        }
    }
    return data;
}
