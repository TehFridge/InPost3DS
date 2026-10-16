#ifndef DATA_H
#define DATA_H

#include <citro2d.h>

#define GFX_COLOR_RGBA(r, g, b, a) C2D_Color32(r, g, b, a)

typedef struct {
    C2D_SpriteSheet sheet; 
    C2D_Image image;       
    float width;
    float height;
    int cacheIndex;
} GFX_IMAGE;

typedef void* GFX_TEXTBUF; 

typedef struct {
    C2D_Text c2d_text; 
    const char* str;   // do rayliba
} GFX_TEXT;

GFX_IMAGE* GFX_LoadTexture(const char* path, int pos_in_sheet);
void GFX_FreeTexture(GFX_IMAGE* tex);

GFX_TEXTBUF GFX_TextBufNew(size_t size);
void GFX_TextBufClear(GFX_TEXTBUF buf);
void GFX_TextBufDelete(GFX_TEXTBUF buf);

void GFX_TextParse(GFX_TEXT* text, GFX_TEXTBUF buf, const char* str);
void GFX_TextOptimize(GFX_TEXT* text);
void GFX_TextGetDimensions(const GFX_TEXT* text, float scaleX, float scaleY, float* outWidth, float* outHeight);
void FS_CheckIfInPostIstnieje();

extern u32 kDown;
extern u32 kHeld;

void INPUT_Scan();
void INPUT_Setup();
#endif