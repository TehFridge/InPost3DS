#ifndef DRAWING_H
#define DRAWING_H

#include "data.h"
#include "../scenes/scene_manager.h"
extern C3D_RenderTarget* left;
extern C3D_RenderTarget* right;
extern C3D_RenderTarget* bottom;

typedef enum {
    GFX_ALIGN_LEFT = 0,
    GFX_ALIGN_CENTER,
    GFX_ALIGN_RIGHT
} GFX_TEXT_ALIGN;

void GFX_DrawImageAt(GFX_IMAGE* texture, float x, float y, float depth, C2D_ImageTint *tint, float scalex, float scaley);

void GFX_DrawText(const GFX_TEXT* text, float x, float y, float depth, float scaleX, float scaleY, GFX_TEXT_ALIGN align, u32 color);
void GFX_DrawTextWrapped(const GFX_TEXT* text, float x, float y, float depth, float scaleX, float scaleY, GFX_TEXT_ALIGN align, u32 color, float wrapWidth);

void GFX_DrawShadowedText(const GFX_TEXT* text, float x, float y, float depth, float scaleX, float scaleY, GFX_TEXT_ALIGN align, u32 color, u32 shadowColor);
void GFX_DrawShadowedTextWrapped(const GFX_TEXT* text, float x, float y, float depth, float scaleX, float scaleY, GFX_TEXT_ALIGN align, u32 color, u32 shadowColor, float wrapWidth);

void GFX_DrawRectSolid(float x, float y, float depth, int w, int h, u32 color);
void GFX_InitGfx();

// 0 - left
// 1 - right
void GFX_BeginSceneTop(int side, bool should_clear);

// stricte 3ds only 
void GFX_BeginSceneBottom();

void GFX_RenderFrame();

void GFX_DrawRectangle(float x, float y, float depth, float w, float h, u32 top_left, u32 top_right, u32 bottom_left, u32 bottom_right);

void GFX_DrawCircleSolid(float x, float y, float depth, float radius, u32 color);

void GFX_DrawLine(float x1, float y1, u32 color0, float x2, float y2, u32 color1, float thick, float depth);
#endif