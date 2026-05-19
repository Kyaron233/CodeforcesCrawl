#include "utils/output.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "utils/logger.h"
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#ifdef _WIN32
#define OUTPUT_DIR "output\\"
#else
#define OUTPUT_DIR "output/"
#endif

static int ensure_output_dir_exists(void) {
#ifdef _WIN32
    if (_mkdir(OUTPUT_DIR) == 0 || errno == EEXIST) {
        return 1;
    }
#else
    if (mkdir(OUTPUT_DIR, 0777) == 0 || errno == EEXIST) {
        return 1;
    }
#endif

    fprintf(stderr, "failed to create output dir: %s (%s)\n", OUTPUT_DIR, strerror(errno));
    return 0;
}


int output_json(const cJSON* root, const char* filename) {
    FILE* out = NULL;
    char* json_text = NULL;
    int ok = 0;

    if (root == NULL || filename == NULL || filename[0] == '\0') {
        return 0;
    }

    json_text = cJSON_Print(root);
    if (json_text == NULL) {
        return 0;
    }

    out = fopen(filename, "w");
    char msg[100];
    if (out == NULL) {
        sprintf(msg,"无法打开指定文件：%s",filename);
        log_message(LOG_ERROR,msg);
        cJSON_free(json_text);
        return 0;
    }

    if (fputs(json_text, out) != EOF) {
        sprintf(msg,"%s写入成功",filename);
        log_message(LOG_INFO,msg);
        ok = 1;
    }
    else {
        sprintf(msg,"%s写入失败",filename);
        log_message(LOG_ERROR,msg);
    }

    fclose(out);
    cJSON_free(json_text);
    return ok;
}

int output_json_with_username(const cJSON* root, const char* username, const char* filename) {
    char complete_filename[512];
    int written = 0;

    if (root == NULL || username == NULL || username[0] == '\0' || filename == NULL || filename[0] == '\0') {
        return 0;
    }

    if (!ensure_output_dir_exists()) {
        return 0;
    }

    written = snprintf(complete_filename, sizeof(complete_filename), "%s%s_%s", OUTPUT_DIR, username, filename);
    if (written < 0 || (size_t)written >= sizeof(complete_filename)) {
        return 0;
    }

    return output_json(root, complete_filename);
}

int output_rawstring_with_username(const char* raw, const char* username, const char* filename) {
    char complete_filename[512];
    char msg[100];
    int written = 0;
    FILE* out = NULL;
    int ok = 0;

    if (raw == NULL || username == NULL || username[0] == '\0' || filename == NULL || filename[0] == '\0') {
        return 0;
    }

    if (!ensure_output_dir_exists()) {
        return 0;
    }

    written = snprintf(complete_filename, sizeof(complete_filename), "%s%s_%s", OUTPUT_DIR, username, filename);
    if (written < 0 || (size_t)written >= sizeof(complete_filename)) {
        return 0;
    }

    out = fopen(complete_filename, "w");
    if (out == NULL) {
        return 0;
    }

    if (fputs(raw, out) != EOF) {
        ok = 1;
    }

    fclose(out);
    if(ok) {
        sprintf(msg,"output_rawstring成功执行！");
        log_message(LOG_INFO,msg);
    }
    else {
        sprintf(msg,"output_rawstring出错！");
        log_message(LOG_ERROR,msg);
    }
    return ok;
}
void append_user_list(const char* username) {
    char path[512];
    FILE* userlist = NULL;
    char* buffer = NULL;
    long size = 0;
    cJSON* root = NULL;
    int found = 0;
    char msg[128];

    if (username == NULL || username[0] == '\0') {
        return;
    }

    if (!ensure_output_dir_exists()) {
        return;
    }

    {
        int written = snprintf(path, sizeof(path), "%sUser.json", OUTPUT_DIR);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            return;
        }
    }

    userlist = fopen(path, "rb");
    if (userlist != NULL) {
        if (fseek(userlist, 0, SEEK_END) == 0) {
            size = ftell(userlist);
        }
        rewind(userlist);

        if (size > 0) {
            buffer = (char*)malloc((size_t)size + 1);
            if (buffer == NULL) {
                fclose(userlist);
                return;
            }
            if (fread(buffer, 1, (size_t)size, userlist) != (size_t)size) {
                fclose(userlist);
                free(buffer);
                return;
            }
            buffer[size] = '\0';
            root = cJSON_Parse(buffer);
        }

        fclose(userlist);
    } else if (errno != ENOENT) {
        snprintf(msg, sizeof(msg), "无法打开%s: %s", path, strerror(errno));
        log_message(LOG_ERROR, msg);
        return;
    }

    if (size > 0) {
        if (root == NULL || !cJSON_IsArray(root)) {
            snprintf(msg, sizeof(msg), "源文件%s不是JSON数组", path);
            log_message(LOG_ERROR, msg);
            cJSON_Delete(root);
            free(buffer);
            return;
        }
    } else if (root == NULL) {
        root = cJSON_CreateArray();
        if (root == NULL) {
            free(buffer);
            return;
        }
    }

    {
        cJSON* item = NULL;
        cJSON_ArrayForEach(item, root) {
            if (cJSON_IsString(item) && item->valuestring != NULL &&
                strcmp(item->valuestring, username) == 0) {
                found = 1;
                log_message(LOG_WARNING,"当前用户已在列表中");
                break;
            }
        }
    }

    if (!found) {
        cJSON_AddItemToArray(root, cJSON_CreateString(username));
    }

    output_json(root, path);

    snprintf(msg, sizeof(msg), "当前用户数：%d", cJSON_GetArraySize(root));
    log_message(LOG_INFO, msg);

    cJSON_Delete(root);
    free(buffer);
}