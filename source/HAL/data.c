#include <stdlib.h>
#include <citro2d.h>
#include "data.h"

#define MAX_CACHE_ENTRIES 32

typedef struct {
    char path[128];
    C2D_SpriteSheet sheet;
    int refCount;
} CacheEntry;

static CacheEntry sheetCache[MAX_CACHE_ENTRIES];
static int cacheCount = 0;


static int GetSheetFromCache(const char* path) {

    for (int i = 0; i < cacheCount; i++) {
        if (strcmp(sheetCache[i].path, path) == 0) {
            sheetCache[i].refCount++;
            return i;
        }
    }

    if (cacheCount >= MAX_CACHE_ENTRIES) return -1; 

    C2D_SpriteSheet newSheet = C2D_SpriteSheetLoad(path);
    if (!newSheet) return -1;

    int newIdx = cacheCount++;
    strncpy(sheetCache[newIdx].path, path, 127);
    sheetCache[newIdx].sheet = newSheet;
    sheetCache[newIdx].refCount = 1;

    return newIdx;
}

GFX_IMAGE* GFX_LoadTexture(const char* path, int pos_in_sheet) {
    int cacheIdx = GetSheetFromCache(path);
    if (cacheIdx == -1) return NULL;

    GFX_IMAGE* newTex = (GFX_IMAGE*)malloc(sizeof(GFX_IMAGE));
    if (newTex == NULL) return NULL;

    newTex->cacheIndex = cacheIdx;
    newTex->sheet = sheetCache[cacheIdx].sheet;
    newTex->image = C2D_SpriteSheetGetImage(newTex->sheet, pos_in_sheet);

    if (newTex->image.subtex) {
        newTex->width = (float)newTex->image.subtex->width;
        newTex->height = (float)newTex->image.subtex->height;
    }

    return newTex;
}

void GFX_FreeTexture(GFX_IMAGE* tex) {
    if (!tex) return;

    int idx = tex->cacheIndex;
    sheetCache[idx].refCount--;

    if (sheetCache[idx].refCount <= 0) {
        C2D_SpriteSheetFree(sheetCache[idx].sheet);
        
        sheetCache[idx] = sheetCache[cacheCount - 1];
        cacheCount--;
    }

    free(tex);
}

GFX_TEXTBUF GFX_TextBufNew(size_t size) {
    return (GFX_TEXTBUF)C2D_TextBufNew(size);
}

void GFX_TextBufClear(GFX_TEXTBUF buf) {
    if (buf) C2D_TextBufClear((C2D_TextBuf)buf);
}

void GFX_TextBufDelete(GFX_TEXTBUF buf) {
    if (buf) C2D_TextBufDelete((C2D_TextBuf)buf);
}

void GFX_TextParse(GFX_TEXT* text, GFX_TEXTBUF buf, const char* str) {
    text->str = str; // do rayliba
    C2D_TextParse(&text->c2d_text, (C2D_TextBuf)buf, str);
}

void GFX_TextOptimize(GFX_TEXT* text) {
    C2D_TextOptimize(&text->c2d_text);
}

void GFX_TextGetDimensions(const GFX_TEXT* text, float scaleX, float scaleY, float* outWidth, float* outHeight) {
    C2D_TextGetDimensions(&text->c2d_text, scaleX, scaleY, outWidth, outHeight);
}
Result czyFolderIstnieje(FS_Archive archive, FS_Path path)
{
    Handle dirHandle = 0;
    Result res;

    res = FSUSER_OpenDirectory(&dirHandle, archive, path);

    if (R_SUCCEEDED(res))
    {
        FSDIR_Close(dirHandle);
        return 0; 
    }

    res = FSUSER_CreateDirectory(archive, path, 0);

    return res;
}
void FS_CheckIfInPostIstnieje(){
    fsInit();
    FS_Archive sdmcArchive;
    FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    FS_Path folderPath = fsMakePath(PATH_ASCII, "/3ds/InPost3DS");
    czyFolderIstnieje(sdmcArchive, folderPath);

    FSUSER_CloseArchive(sdmcArchive);
    fsExit();
}

u32 kDown;
u32 kHeld;

void INPUT_Scan(){
    hidScanInput();
    kDown = hidKeysDown();
    kHeld = hidKeysHeld();
}

// na 3dsie zbytnio to nie ma co setupować, bardziej na switchu
void INPUT_Setup(){
    return;
}