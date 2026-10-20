#ifndef LANG_H
#define LANG_H

#include "../libs/cJSON.h"
#include "utils.h"

extern const char* lang_strings[128];

typedef enum {
    PL,
    EN,
} Language;

extern Language currentLang;


void loadLanguage(Language lang);

#endif