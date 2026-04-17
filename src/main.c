#include <stdio.h>
#include "cJSON.h"
#include <curl/curl.h>
#include "include/utils.h"
#define URL_TEST "http://httpbin.org/ip"


int main() {
    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        printf("curl初始化失败\n");
        return 1;
    }
    char buf[2000]; 
    size_t rcv;
    curl_easy_setopt(curl,CURLOPT_CONNECT_ONLY,1l);
    curl_easy_setopt(curl, CURLOPT_URL,URL_TEST);
    CURLcode res = curl_easy_recv(curl,buf,2000,&rcv);
    // CURLcode res = curl_easy_perform(curl);

    printf("request done\n");
    printf("curl result code: %d\n", (int)res);
    if (res != CURLE_OK) {
        printf("curl error: %s\n", curl_easy_strerror(res));
    }
    utiltest();
    curl_easy_cleanup(curl);
    return 0;
}
