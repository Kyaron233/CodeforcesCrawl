#include <stdio.h>
#include <curl/curl.h>

#define URL_TEST "http://httpbin.org/ip"

static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    (void)userp;
    fwrite(contents, 1, total, stdout);
    return total;
}

int main(void) {
    CURL *curl = curl_easy_init();
    CURLcode res;

    if (curl == NULL) {
        printf("curl初始化失败\n");
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, URL_TEST);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

    res = curl_easy_perform(curl);
    printf("\nrequest done\n");

    if (res != CURLE_OK) {
        printf("curl error: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    return res == CURLE_OK ? 0 : 1;
}