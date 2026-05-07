#include "utils/output.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

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
    if (out == NULL) {
        cJSON_free(json_text);
        return 0;
    }

    if (fputs(json_text, out) != EOF) {
        ok = 1;
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
    return ok;
}