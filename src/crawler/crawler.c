#include<curl/curl.h>
#include "utils/logger.h"
size_t write_callback(char* data,size_t size,size_t nmemb,void* userp){
    size_t realsize = size * nmemb;
    (void)userp;

}

void getMatchList(CURL* curl) {
    if(curl == NULL){
        log_message(ERROR, "curl初始化失败");
        return;
    }  
    


    curl_easy_cleanup(curl);
}