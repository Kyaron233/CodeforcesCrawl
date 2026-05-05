#include "utils/output.h"
#include <stdio.h>

#ifdef _WIN32
#define OUTPUT_DIR "output\\"
#else
#define OUTPUT_DIR "output/"
#endif



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

    written = snprintf(complete_filename, sizeof(complete_filename), "%s%s_%s", OUTPUT_DIR, username, filename);
    if (written < 0 || (size_t)written >= sizeof(complete_filename)) {
        return 0;
    }

    return output_json(root, complete_filename);
}