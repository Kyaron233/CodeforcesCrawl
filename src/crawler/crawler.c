#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/logger.h"
#include "crawler/crawler.h"
#include "core.h"

static CURL *global_curl = NULL; 
static char apikey[200] = {'\0'};

static int file_exists(const char* path) {
    FILE* file = NULL;

    if (path == NULL || path[0] == '\0') {
        return 0;
    }

    file = fopen(path, "rb");
    if (file != NULL) {
        fclose(file);
        return 1;
    }

    return 0;
}

static const char* resolve_ca_bundle_path(void) {
    static char cached_path[512];
    static int checked = 0;
    const char* env = NULL;

    if (checked) {
        return cached_path[0] != '\0' ? cached_path : NULL;
    }

    checked = 1;

    env = getenv("CURL_CA_BUNDLE");
    if (env != NULL && env[0] != '\0' && file_exists(env)) {
        snprintf(cached_path, sizeof(cached_path), "%s", env);
        return cached_path;
    }

    env = getenv("SSL_CERT_FILE");
    if (env != NULL && env[0] != '\0' && file_exists(env)) {
        snprintf(cached_path, sizeof(cached_path), "%s", env);
        return cached_path;
    }

    const char* candidates[] = {
        "certs/cacert.pem",
        "certs/ca-bundle.crt",
        "certs/curl-ca-bundle.crt",
        "../certs/cacert.pem",
        "../certs/ca-bundle.crt",
        "../../certs/cacert.pem",
        "../../certs/ca-bundle.crt",
        "resources/cacert.pem",
        "../resources/cacert.pem",
        "../../resources/cacert.pem"
    };

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        if (file_exists(candidates[i])) {
            snprintf(cached_path, sizeof(cached_path), "%s", candidates[i]);
            return cached_path;
        }
    }

    return NULL;
}

static void apply_ca_bundle_option(CURL* curl) {
    const char* ca_bundle = resolve_ca_bundle_path();

    if (ca_bundle != NULL) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_bundle);
        return;
    }

#ifdef WIN32
    static int warned = 0;
    if (!warned) {
        log_message(LOG_WARNING, "CA bundle not found; set CURL_CA_BUNDLE or place certs/cacert.pem.");
        warned = 1;
    }
#endif
}
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

static void shrink_data(Data* data) {
    if (data == NULL || data->chunk == NULL) {
        return;
    }

    size_t exact = data->size + 1;
    if (data->cap == exact) {
        return;
    }

    char* new_chunk = realloc(data->chunk, exact);
    if (new_chunk != NULL) {
        data->chunk = new_chunk;
        data->cap = exact;
    }
}

size_t write_callback(char* data,size_t size,size_t nmemb,void* userp){
    size_t realsize = size * nmemb;
    Data *userdata = (Data*)userp;

    char msg[1000] = "write_callback working";

    size_t required = userdata->size + realsize + 1; // +1 for explicit '\0'
    if (required > userdata->cap) {
        size_t new_cap = userdata->cap + realsize + 16;
        if (new_cap < required) {
            new_cap = required;
        }

        char* new_userdata_ptr = realloc(userdata->chunk, new_cap);
        if(new_userdata_ptr==NULL){
            sprintf(msg,"[write_callback]:爬取数据时分配内存失败！");
            log_message(LOG_ERROR,msg);
            return 0;
        }
        userdata->chunk = new_userdata_ptr;
        userdata->cap = new_cap;
    }
    memcpy(&(userdata->chunk[userdata->size]),data,realsize);
    userdata->size = userdata->size + realsize;
    userdata->chunk[userdata->size] = 0;

    return realsize;
}
void set_custom_options(CURL *curl) {
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 设置超时时间为10秒
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // 允许重定向
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl,CURLOPT_SSL_OPTIONS,CURLSSLOPT_NATIVE_CA);
    apply_ca_bundle_option(curl);
}
CURL* getCurl(){
    if(global_curl!=NULL) return global_curl;
    else{
        log_message(LOG_INFO,"全局curl句柄失效，正在尝试初始化");
        global_curl = curl_easy_init();
        if(global_curl==NULL) {
            log_message(LOG_ERROR,"无法初始化curl！");
            return NULL;
        }
        return global_curl;
    }
}
// “2.	抓取比赛列表和用户参加比赛的列表” 
// 获取比赛列表，注意并不全是用户参加的比赛。
Data getContestList(Status* status) {
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
    log_message(LOG_INFO,"正在执行getContestList...");
    curl_easy_reset(curl);
    set_custom_options(curl);
    curl_easy_setopt(curl, CURLOPT_URL, BASE_URL "/contest.list?gym=false"); // 不获取gym的数据
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
    shrink_data(&data);
    return data;
}

Data getUserAttendedContestList(Status* status,char* username){
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
    char completed_url[100];
    sprintf(completed_url,"%s/user.rating?handle=%s",BASE_URL,username);
    log_message(LOG_INFO,"正在执行getUserAttendedContestList...");
    curl_easy_reset(curl);
    set_custom_options(curl);
    curl_easy_setopt(curl, CURLOPT_URL,completed_url); // https://codeforces.com/api/user.rating?handle={username}
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
    shrink_data(&data);
    return data;
}

Data getUserStatus(Status* status,char* username){
    Data data = {0};
    init_status(status);

    if (username == NULL || username[0] == '\0') {
        if (status != NULL) {
            status->status = STATUS_INTERNAL_ERROR;
            status->curl_code = CURLE_OK;
            status->msg = "用户名为空";
        }
        return data;
    }

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
    char completed_url[200];
    int written = snprintf(completed_url, sizeof(completed_url), "%s/user.status?handle=%s", BASE_URL, username);
    if (written < 0 || written >= (int)sizeof(completed_url)) {
        if (status != NULL) {
            status->status = STATUS_INTERNAL_ERROR;
            status->curl_code = CURLE_OK;
            status->msg = "请求URL过长";
        }
        return data;
    }

    log_message(LOG_INFO,"正在执行getUserStatus...");
    curl_easy_reset(curl);
    set_custom_options(curl);
    curl_easy_setopt(curl, CURLOPT_URL, completed_url); // https://codeforces.com/api/user.status?handle={username}
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void*)&data);
    if (status != NULL) {
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, status->curl_error);
    }

    code = curl_easy_perform(curl);

    if(code != CURLE_OK) {
        if (status != NULL) {
            status->status = STATUS_CURL_ERROR;
            status->curl_code = code;
            fill_response_meta(curl, &status->resp);
            status->msg = (status->curl_error[0] != '\0') ? status->curl_error : curl_easy_strerror(code);
        }
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
    shrink_data(&data);
    return data;
}

Data getUserInfo(Status* status, char* username) {
    Data data = {0};
    init_status(status);

    if (username == NULL || username[0] == '\0') {
        if (status != NULL) {
            status->status = STATUS_INTERNAL_ERROR;
            status->curl_code = CURLE_OK;
            status->msg = "用户名为空";
        }
        return data;
    }

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
    char completed_url[200];
    int written = snprintf(completed_url, sizeof(completed_url), "%s/user.info?handles=%s", BASE_URL, username);
    if (written < 0 || written >= (int)sizeof(completed_url)) {
        if (status != NULL) {
            status->status = STATUS_INTERNAL_ERROR;
            status->curl_code = CURLE_OK;
            status->msg = "请求URL过长";
        }
        return data;
    }

    log_message(LOG_INFO,"正在执行getUserInfo...");
    curl_easy_reset(curl);
    set_custom_options(curl);
    curl_easy_setopt(curl, CURLOPT_URL, completed_url); // https://codeforces.com/api/user.info?handles={username}
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void*)&data);
    if (status != NULL) {
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, status->curl_error);
    }

    code = curl_easy_perform(curl);

    if(code != CURLE_OK) {
        if (status != NULL) {
            status->status = STATUS_CURL_ERROR;
            status->curl_code = code;
            fill_response_meta(curl, &status->resp);
            status->msg = (status->curl_error[0] != '\0') ? status->curl_error : curl_easy_strerror(code);
        }
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
    shrink_data(&data);
    return data;
}



Data getUserRating(Status* status,char* username){
    Data data = {0};
    init_status(status);

    if (username == NULL || username[0] == '\0') {
        if (status != NULL) {
            status->status = STATUS_INTERNAL_ERROR;
            status->curl_code = CURLE_OK;
            status->msg = "用户名为空";
        }
        return data;
    }

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
    char completed_url[200];
    int written = snprintf(completed_url, sizeof(completed_url), "%s/user.rating?handle=%s", BASE_URL, username);
    if (written < 0 || written >= (int)sizeof(completed_url)) {
        if (status != NULL) {
            status->status = STATUS_INTERNAL_ERROR;
            status->curl_code = CURLE_OK;
            status->msg = "请求URL过长";
        }
        return data;
    }

    log_message(LOG_INFO,"正在执行getUserRating...");
    curl_easy_reset(curl);
    set_custom_options(curl);
    curl_easy_setopt(curl, CURLOPT_URL, completed_url); // https://codeforces.com/api/user.rating?handle={username}
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void*)&data);
    if (status != NULL) {
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, status->curl_error);
    }

    code = curl_easy_perform(curl);

    if(code != CURLE_OK) {
        if (status != NULL) {
            status->status = STATUS_CURL_ERROR;
            status->curl_code = code;
            fill_response_meta(curl, &status->resp);
            status->msg = (status->curl_error[0] != '\0') ? status->curl_error : curl_easy_strerror(code);
        }
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
    shrink_data(&data);
    return data;
}