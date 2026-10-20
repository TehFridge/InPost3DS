#include <3ds.h>
#include <citro2d.h>
#include "scene_searchpaczkomat.h"
#include "scene_manager.h"
#include "scene_init.h"
#include "../main.h"
#include "../utils/sprites.h"
#include "../utils/button.h"
#include "../utils/cwav_shit.h"
#include "../utils/anim.h"
#include "../inpost/inpost_api.h"
#include "../utils/utils.h"
#include "../utils/lang.h"

static u64 lastTick = 0;
static float dt = 0.0f;
static float dtScale = 1.0f;
static float spinnerAngle = 0.0f;
static float spinnerSpeed = 180.0f;

static Ease flash_ease;
static Ease results_in_ease;

bool search_parsed;
bool search_initiated;

static GFX_TEXTBUF searchBuf;
static GFX_TEXT S_text;

static Button result_btns[MAX_PACZKOMAT_RESULTS];
static GFX_TEXT result_name_texts[MAX_PACZKOMAT_RESULTS];
static GFX_TEXT result_desc_texts[MAX_PACZKOMAT_RESULTS];

static float scroll_y = 0.0f;
static float max_scroll_y = 0.0f;
static float touch_drag_start_y = 0.0f;
static float drag_distance = 0.0f;
static bool is_dragging = false;
static float scroll_velocity = 0.0f;

bool returning_to_nadaj = false;

void sceneSearchpaczkomatInit(void) {
    returning_to_nadaj = false;
    search_initiated = false;
    flash_ease = initEase(1.2f, EASE_OUT);
    search_parsed = false;
    paczkomat_search.done = false;

    lastTick = svcGetSystemTick();
    dt = 0.0f;
    dtScale = 1.0f;
    
    scroll_y = 0.0f;
    max_scroll_y = 0.0f;
    is_dragging = false;
    
    searchBuf = GFX_TextBufNew(4096);

    GFX_TextParse(&S_text, searchBuf, lang_strings[30]);
    GFX_TextOptimize(&S_text);

}

static bool prevTouchHeld = false;
static float cached_touch_x = 0.0f;
static float cached_touch_y = 0.0f;

void sceneSearchpaczkomatUpdate(uint32_t kDown, uint32_t kHeld) {
    updateEase(&flash_ease);
    u64 currentTick = svcGetSystemTick();
    dt = (float)(currentTick - lastTick) / CPU_TICKS_PER_MSEC / 1000.0f;
    lastTick = currentTick;

    if (dt > 0.1f) dt = 0.1f;
    if (dt < 0.001f) dt = 0.001f;
    dtScale = dt * 60.0f;

    touchPosition touch;
    hidTouchRead(&touch);

    bool touchDown = (kDown & KEY_TOUCH) != 0;
    bool touchHeld = (kHeld & KEY_TOUCH) != 0;
    bool touchUp   = (!touchHeld && prevTouchHeld);
    prevTouchHeld  = touchHeld;

    if (touchDown || touchHeld) {
        cached_touch_x = touch.px;
        cached_touch_y = touch.py;
    }

    if (touchUp) {
        touch.px = cached_touch_x;
        touch.py = cached_touch_y;
    }

    if (search_parsed) {
        circlePosition pos;
        hidCircleRead(&pos);

        if (abs(pos.dy) > 20) {
            float norm_y = (float)pos.dy / 150.0f;
            scroll_y -= norm_y * 8.0f * dtScale;
        }

        if (kHeld & KEY_DUP)    scroll_y -= 8.0f * dtScale;
        if (kHeld & KEY_DDOWN)  scroll_y += 8.0f * dtScale;
    }

    if (search_parsed) {
        if (paczkomat_search_count < 1) {
            search_initiated = false; 
            search_parsed = false;
            paczkomat_search.done = false;
            for (int i = 0; i < paczkomat_search_count; i++) {
                Destroy_button(&result_btns[i]);
            }
        }
        if (scroll_y < 0.0f) {
            scroll_y = 0.0f;
        }
        if (scroll_y > max_scroll_y) {
            scroll_y = max_scroll_y;
        }
    }

    if (flash_ease.value == 1.0f && !search_initiated) {
        search_initiated = true;
        char mybuf[256];
        SwkbdState swkbd;
        SwkbdButton button = SWKBD_BUTTON_NONE;
        
        swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, 59);
        swkbdSetHintText(&swkbd, lang_strings[69]);
        swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

        button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
        
        if (button != SWKBD_BUTTON_NONE && button != SWKBD_BUTTON_LEFT) { 
            searchPaczkomat(mybuf);
        } else {
            search_initiated = false; 
        }
    }

    if (search_initiated && paczkomat_search.done && !search_parsed) {
        parsePaczkomatSearch(paczkomat_search.data);
        search_parsed = true;

        for (int i = 0; i < paczkomat_search_count; i++) {
            Create_button(&result_btns[i], 400.0f, 7.0f + (57.0f * i), 311, 54, paczka_size, paczka_size_selected, GFX_COLOR_RGBA(255,255,255,0), GFX_COLOR_RGBA(255,255,255,0), 1.0f);

            GFX_TextParse(&result_name_texts[i], searchBuf, paczkomat_search_results[i].name);
            GFX_TextOptimize(&result_name_texts[i]);

            char descBuf[128];
            snprintf(descBuf, sizeof(descBuf), "%s %s, %s", paczkomat_search_results[i].street, paczkomat_search_results[i].buildingNumber, paczkomat_search_results[i].city);
            GFX_TextParse(&result_desc_texts[i], searchBuf, descBuf);
            GFX_TextOptimize(&result_desc_texts[i]);
        }
        
        float total_height = (paczkomat_search_count * 57.0f) + 14.0f;
        max_scroll_y = (total_height > 240.0f) ? (total_height - 240.0f) : 0.0f;
        
        results_in_ease = initEase(0.8f, EASE_OUT);
    }

    if (search_parsed) {
        if (kDown & KEY_B) {
            playCwav(4, true);
            search_initiated = false; 
            search_parsed = false;
            paczkomat_search.done = false;
            for (int i = 0; i < paczkomat_search_count; i++) {
                Destroy_button(&result_btns[i]);
            }
            returning_to_nadaj = false;
            sceneManagerSwitchTo(SCENE_NADAJ);
        }
        updateEase(&results_in_ease);

        float target_x =
            420.0f - (410.0f * results_in_ease.value);

        for (int i = 0; i < paczkomat_search_count; i++) {

            result_btns[i].x = target_x;
            result_btns[i].y =
                7.0f + (57.0f * i) - scroll_y;

            Update_button(
                &result_btns[i],
                touch,
                touchDown,
                touchHeld,
                touchUp
            );

            if (result_btns[i].clicked) {
                playCwav(4, true);

                strncpy(
                    reciever_info.boxMachineName,
                    paczkomat_search_results[i].name,
                    sizeof(reciever_info.boxMachineName) - 1
                );

                reciever_info.boxMachineName[
                    sizeof(reciever_info.boxMachineName) - 1
                ] = '\0';
                returning_to_nadaj = true;
                sceneManagerSwitchTo(SCENE_NADAJ);
            }
        }
    }
}

#define SPINNER_DOT_COUNT 12
static void drawSpinner(float cx, float cy, float alphaVal) {
    if (alphaVal <= 0.0f) return;
    float bigRadius = 40.0f;
    float dotSize = 8.0f;
    float shadowOffset = 2.0f;
    
    spinnerAngle += spinnerSpeed * dt;
    if (spinnerAngle >= 360.0f) spinnerAngle -= 360.0f;
    
    for (int i = 0; i < SPINNER_DOT_COUNT; i++) {
        float angleDeg = spinnerAngle + (360.0f / SPINNER_DOT_COUNT) * i;
        float angleRad = angleDeg * M_PI / 180.0f;
        float x = cx + bigRadius * cosf(angleRad);
        float y = cy + bigRadius * sinf(angleRad);
        float dotAlpha = (0.3f + 0.7f * (float)i / SPINNER_DOT_COUNT) * alphaVal;
        u8 a = (u8)(dotAlpha * 255);
        u32 shadowColor = GFX_COLOR_RGBA(0, 0, 0, a / 2);
        GFX_DrawRectangle(x - dotSize / 2.0f + shadowOffset, y - dotSize / 2.0f + shadowOffset, 0.98f, dotSize, dotSize, shadowColor, shadowColor, shadowColor, shadowColor);
        u32 dotColor = GFX_COLOR_RGBA(255, 255, 255, a);
        GFX_DrawRectangle(x - dotSize / 2.0f, y - dotSize / 2.0f, 0.99f, dotSize, dotSize, dotColor, dotColor, dotColor, dotColor);
    }
}

static float scroll_speed2 = 0.5f;
static float bg_x2 = 0.0f;

static void drawHomeSceneContent(float offset) {
    GFX_DrawImageAt(bg_top, bg_x2 - 560 + offset, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
    GFX_DrawImageAt(bg_top, bg_x2 + offset, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
    switch(VOICEACT) {
        case 1: 
            GFX_DrawImageAt(kun_idle, 10.0f, 0.0f, 0.6f, NULL, 1.0f, 1.0f);
            break;
        case 2:
            GFX_DrawImageAt(chan_idle, 10.0f, 0.0f, 0.6f, NULL, 1.0f, 1.0f);
            break;  
        case 3:
            GFX_DrawImageAt(arhn_explain, 10.0f, 0.0f, 0.6f, NULL, 1.0f, 1.0f);
            break;  
        default: 
            GFX_DrawImageAt(chan_idle, 10.0f, 0.0f, 0.6f, NULL, 1.0f, 1.0f);
            break;  
    }
    GFX_DrawShadowedText(
        &S_text,
        270.0f,
        50.0f,
        0.6f,
        0.65f,
        0.65f, GFX_ALIGN_CENTER,
        GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff),
        GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff)
    );
    GFX_DrawRectangle(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f));
}

void sceneSearchpaczkomatRender(void) {
    bg_x2 -= scroll_speed2 * dtScale;
    if (bg_x2 <= -80.0f) bg_x2 += 80.0f;

    GFX_BeginSceneTop(0, true);
    drawHomeSceneContent(0.0f);

    if (slider > 0.0f) {
        GFX_BeginSceneTop(1, true);
        drawHomeSceneContent(slider * 5.0f);
    }

    GFX_BeginSceneBottom();
    GFX_DrawImageAt(bg_bottom, bg_x2, 0.0f, 0.0f, NULL, 1.0f, 1.0f);

    if (!search_parsed && search_initiated) {
        drawSpinner(160.0f, 120.0f, 255.0f);
    }

    if (search_parsed) {
        for (int i = 0; i < paczkomat_search_count; i++) {
            if (result_btns[i].x < 400.0f && result_btns[i].y > -60.0f && result_btns[i].y < 240.0f) {
                Draw_button(&result_btns[i]);
                GFX_DrawText(&result_name_texts[i], result_btns[i].x + 10.0f, result_btns[i].y + 8.0f, 1.0f, 0.8f, 0.8f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
                GFX_DrawText(&result_desc_texts[i], result_btns[i].x + 10.0f, result_btns[i].y + 32.0f, 1.0f, 0.45f, 0.45f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            }
        }
    }

    GFX_DrawRectangle(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f));
}

void sceneSearchpaczkomatExit(void) {
    GFX_TextBufClear(searchBuf); 
}