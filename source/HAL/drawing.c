#include "drawing.h"

C3D_RenderTarget* left;
C3D_RenderTarget* right;
C3D_RenderTarget* bottom;
static u32 GetC2DAlignFlag(GFX_TEXT_ALIGN align) {
    switch (align) {
        case GFX_ALIGN_CENTER: return C2D_AlignCenter;
        case GFX_ALIGN_RIGHT:  return C2D_AlignRight;
        case GFX_ALIGN_LEFT:
        default:               return C2D_AlignLeft;
    }
}

void GFX_DrawImageAt(GFX_IMAGE* texture, float x, float y, float depth, C2D_ImageTint *tint, float scalex, float scaley) {
    C2D_DrawImageAt(texture->image, x, y, depth, tint, scalex, scaley);  
}

void GFX_DrawText(const GFX_TEXT* text, float x, float y, float depth, float scaleX, float scaleY, GFX_TEXT_ALIGN align, u32 color) {
    u32 flags = GetC2DAlignFlag(align) | C2D_WithColor;
    C2D_DrawText(&text->c2d_text, flags, x, y, depth, scaleX, scaleY, color);
}

void GFX_DrawTextWrapped(const GFX_TEXT* text, float x, float y, float depth, float scaleX, float scaleY, GFX_TEXT_ALIGN align, u32 color, float wrapWidth) {
    u32 flags = GetC2DAlignFlag(align) | C2D_WithColor | C2D_WordWrap;
    C2D_DrawText(&text->c2d_text, flags, x, y, depth, scaleX, scaleY, color, wrapWidth);
}

void GFX_DrawShadowedText(const GFX_TEXT* text, float x, float y, float depth, float scaleX, float scaleY, GFX_TEXT_ALIGN align, u32 color, u32 shadowColor) {
    static const float shadowOffsets[4][2] = {
        {0.0f, 1.8f}, {0.0f, -0.7f}, {-1.7f, 0.0f}, {1.8f, 0.0f}
    };

    for (int i = 0; i < 4; i++) {
        GFX_DrawText(text, x + shadowOffsets[i][0], y + shadowOffsets[i][1], depth, scaleX, scaleY, align, shadowColor);
    }
    GFX_DrawText(text, x, y, depth, scaleX, scaleY, align, color);
}

void GFX_DrawShadowedTextWrapped(const GFX_TEXT* text, float x, float y, float depth, float scaleX, float scaleY, GFX_TEXT_ALIGN align, u32 color, u32 shadowColor, float wrapWidth) {
    static const float shadowOffsets[4][2] = {
        {0.0f, 1.8f}, {0.0f, -0.7f}, {-1.7f, 0.0f}, {1.8f, 0.0f}
    };

    for (int i = 0; i < 4; i++) {
        GFX_DrawTextWrapped(text, x + shadowOffsets[i][0], y + shadowOffsets[i][1], depth, scaleX, scaleY, align, shadowColor, wrapWidth);
    }
    GFX_DrawTextWrapped(text, x, y, depth, scaleX, scaleY, align, color, wrapWidth);
}

void GFX_DrawRectSolid(float x, float y, float depth, int w, int h, u32 color){
    C2D_DrawRectSolid(x, y, depth, w, h, color);
}

void GFX_InitGfx(){
    gfxInitDefault();
    PrintConsole topConsole;
    consoleInit(GFX_TOP, &topConsole);
    gfxExit();
    
    gfxInitDefault();
    gfxSet3D(false);
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    
    left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
}


void GFX_BeginSceneTop(int side, bool should_clear){
    switch(side){
        case 0:
            C2D_SceneBegin(left);
            if (should_clear) {
                C2D_TargetClear(left, C2D_Color32(0, 0, 0, 255));
            }
            break;
        case 1:
            C2D_SceneBegin(right);
            if (should_clear) {
                C2D_TargetClear(right, C2D_Color32(0, 0, 0, 255));
            }
            break;
    }
}

void GFX_BeginSceneBottom(){
    C2D_SceneBegin(bottom);
    C2D_TargetClear(bottom, C2D_Color32(0, 0, 0, 255));
}

void GFX_RenderFrame(){
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    sceneManagerRender();
    C3D_FrameEnd(0);
}

void GFX_DrawRectangle(float x, float y, float depth, float w, float h, u32 top_left, u32 top_right, u32 bottom_left, u32 bottom_right){
    C2D_DrawRectangle(x, y, depth, w, h, top_left, top_right, bottom_left, bottom_right);
}

void GFX_DrawCircleSolid(float x, float y, float depth, float radius, u32 color){
    C2D_DrawCircleSolid(x, y, depth, radius, color);         
}

void GFX_DrawLine(float x1, float y1, u32 color0, float x2, float y2, u32 color1, float thick, float depth){
    C2D_DrawLine(x1, y1, color0, x2, y2, color1, thick, depth);
}