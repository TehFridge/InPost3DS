#ifndef UTILS_H
#define UTILS_H
#include "../libs/cJSON.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "logs.h"

#define VER_MAJOR (APP_VER >> 10)
#define VER_MINOR ((APP_VER >> 4) & 0x3F) 
#define VER_PATCH (APP_VER & 0x0F)        

void jstrcpy(char *dst, size_t dstsz, cJSON *jstr);
void open_json(const char* path, cJSON** json);
void save_json(const char* path, cJSON* json);
#endif