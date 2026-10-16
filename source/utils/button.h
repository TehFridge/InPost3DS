#ifndef BUTTON_H
#define BUTTON_H
#include "../HAL/data.h"
#include "../HAL/drawing.h"

typedef struct {
    float x, y, width, height;
    bool hovered;
    GFX_IMAGE* image;
    GFX_IMAGE* image_pressed;
    bool disableImageDebug;
    u32 color;
    u32 color_pressed;
    bool wasPressed;
    bool pressed;
    bool clicked;
    bool toggled;
    float depth;
} Button;

void Create_button(Button* btn, float x, float y, float width, float height, GFX_IMAGE* image, GFX_IMAGE* image_pressed, u32 color, u32 color_pressed, float depth);
void Destroy_button(Button* btn);

void Draw_button(Button* btn);
void Update_button(Button* btn, touchPosition touch, bool touchDown, bool touchHeld, bool touchUp);
#endif

