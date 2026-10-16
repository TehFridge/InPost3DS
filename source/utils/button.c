#include "button.h"

void Create_button(Button* btn, float x, float y, float width, float height, GFX_IMAGE* image, GFX_IMAGE* image_pressed, u32 color, u32 color_pressed, float depth) {
    btn->x = x;
    btn->y = y;
    btn->width = width;
    btn->height = height;
    btn->pressed = false;
    btn->image = image;
    btn->image_pressed = image_pressed;
    btn->disableImageDebug = false;
    btn->color = color;
    btn->color_pressed = color_pressed;
    btn->wasPressed = false;
    btn->clicked = false;
    btn->toggled = false;
    btn->depth = depth;
}

void Destroy_button(Button* btn) {
    btn->x = 0;
    btn->y = 0;
    btn->width = 0;
    btn->height = 0;
    btn->pressed = false;
    btn->image = NULL;
    btn->image_pressed = NULL;
    btn->disableImageDebug = false;
    btn->color = GFX_COLOR_RGBA(0, 0, 0, 255);
    btn->color_pressed = GFX_COLOR_RGBA(0, 0, 0, 255);
    btn->wasPressed = false;
    btn->clicked = false;
    btn->toggled = false;
    btn->depth = false;
}

void Draw_button(Button* btn) {
    if ((btn->pressed || btn->hovered) && btn->image_pressed && !btn->disableImageDebug) {
        GFX_DrawImageAt(btn->image_pressed, btn->x, btn->y, btn->depth, NULL, 1.0f, 1.0f);
    } else if ((!btn->pressed || !btn->hovered) && btn->image && !btn->disableImageDebug) {
        GFX_DrawImageAt(btn->image, btn->x, btn->y, btn->depth, NULL, 1.0f, 1.0f);
    } else {
        u32 color = (btn->pressed || btn->hovered) ? btn->color_pressed : btn->color;
        GFX_DrawRectSolid(btn->x, btn->y, btn->depth, btn->width, btn->height, color);
    }
}

void Update_button(Button* btn, touchPosition touch, bool touchDown, bool touchHeld, bool touchUp) {
    bool inside =
        (touch.px >= btn->x && touch.px <= btn->x + btn->width &&
         touch.py >= btn->y && touch.py <= btn->y + btn->height);

    btn->hovered = inside;

    if (touchDown) {
        btn->wasPressed = inside;
    }

    btn->pressed = (btn->wasPressed && inside && touchHeld);

    btn->clicked = false;
    if (touchUp) {
        if (btn->wasPressed && inside) {
            btn->clicked = true;
            btn->toggled = !btn->toggled;
        }
        btn->wasPressed = false; 
    }
}

