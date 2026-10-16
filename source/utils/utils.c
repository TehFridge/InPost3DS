#include "utils.h"

void jstrcpy(char *dst, size_t dstsz, cJSON *jstr){
    if (cJSON_IsString(jstr) && jstr->valuestring) {
        snprintf(dst, dstsz, "%s", jstr->valuestring);
    } else {
        dst[0] = '\0';
    }
}

void open_json(const char* path, cJSON** json)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        log_to_file("[open_json] ERROR: Could not open file %s", path);
        *json = NULL;
        return;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        *json = NULL;
        return;
    }

    long size = ftell(f);
    if (size <= 0) {
        fclose(f);
        *json = NULL;
        return;
    }

    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        log_to_file("[open_json] ERROR: malloc failed");
        fclose(f);
        *json = NULL;
        return;
    }

    size_t read = fread(buffer, 1, size, f);
    fclose(f);

    buffer[read] = '\0';

    *json = cJSON_Parse(buffer);
    free(buffer);

    if (!*json) {
        log_to_file("[open_json] ERROR: JSON parsing failed for file %s", path);
    }
}

void save_json(const char* path, cJSON* json)
{
    char *json_str = cJSON_PrintUnformatted(json);
    if (!json_str)
        return;

    FILE *f = fopen(path, "wb");
    if (!f) {
        free(json_str);
        return;
    }

    fwrite(json_str, 1, strlen(json_str), f);
    fclose(f);

    free(json_str);
}