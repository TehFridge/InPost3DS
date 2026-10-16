#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "../main.h"
#include "../HAL/drawing.h"
#include "../utils/sprites.h"
#include "../utils/cwav_shit.h"
#include "../utils/anim.h"
#include "scene_intro.h"
#include "scene_manager.h"
#include "scene_debug.h"

static float introTimer = 0.0f;
static float flashTimer = 0.0f;
static float fadeTimer = 0.0f;

static bool title2Shown = false;
static bool title4Shown = false;
static bool fadeStarted = false;

#define FLASH_DURATION 1.0f
#define FADE_DURATION 1.0f

static void drawTitle3D(GFX_IMAGE* img) {
    if (!img) return;

    float baseW = (img->width > 0.0f) ? img->width : 400.0f;
    float baseH = (img->height > 0.0f) ? img->height : 240.0f;

    float drawX = 200.0f - (baseW / 2.0f);
    float drawY = 120.0f - (baseH / 2.0f);

    GFX_BeginSceneTop(0, false);
    GFX_DrawImageAt(img, drawX - (3.0f * slider), drawY, 0.1f, NULL, 1.0f, 1.0f);

    if (slider != 0.0f) {
        GFX_BeginSceneTop(1, false);
        GFX_DrawImageAt(img, drawX + (3.0f * slider), drawY, 0.1f, NULL, 1.0f, 1.0f);
    }
}

void sceneIntroInit(void) {
    gfxSet3D(true);

    getCwav(0);
    getCwav(2);
    getCwav(4);

    playCwav(2, true);

    introTimer = 0.0f;
    flashTimer = 0.0f;
    fadeTimer = 0.0f;

    title2Shown = false;
    title4Shown = false;
    fadeStarted = false;
}

void sceneIntroUpdate(uint32_t kDown, uint32_t kHeld) {
    introTimer += 1.0f / 60.0f;

    if (!title2Shown && introTimer >= 0.33f) {
        title2Shown = true;
    }

    if (!title4Shown && introTimer >= 0.65f) {
        title4Shown = true;
        flashTimer = FLASH_DURATION;
    }

    if (flashTimer > 0.0f) {
        flashTimer -= 1.0f / 60.0f;

        if (flashTimer < 0.0f) {
            flashTimer = 0.0f;
        }
    }

    if (title4Shown && !fadeStarted && introTimer >= 2.65f) {
        fadeStarted = true;
        fadeTimer = 0.0f;
    }

    if (fadeStarted) {
        fadeTimer += 1.0f / 60.0f;

        if (fadeTimer >= FADE_DURATION) {
            fadeTimer = FADE_DURATION;
            sceneManagerSwitchTo(SCENE_MAIN_MENU);
        }
    }

#ifdef DEBUG
    if ((kDown & KEY_R) && !fake_paczka_mode) {
        fake_paczka_mode = true;
        playCwav(0, true);
    }

    if ((kDown & KEY_L) && !debugmode) {
        debugmode = true;
        playCwav(0, true);
    }
#endif
}

void sceneIntroRender(void) {
    GFX_BeginSceneTop(0, true);

    if (slider != 0.0f) {
        GFX_BeginSceneTop(1, true);
    }

    drawTitle3D(title_1);

    if (title2Shown) {
        drawTitle3D(title_2);
        drawTitle3D(title_3);
    }

    if (title4Shown) {
        drawTitle3D(title_4);
    }

    if (flashTimer > 0.0f) {
        float alpha = (flashTimer / FLASH_DURATION) * 255.0f;

        GFX_BeginSceneTop(0, false);
        GFX_DrawRectSolid(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255, 255, 255, (uint8_t)alpha));

        if (slider != 0.0f) {
            GFX_BeginSceneTop(1, false);
            GFX_DrawRectSolid(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255, 255, 255, (uint8_t)alpha));
            GFX_BeginSceneTop(0, false);
        }
    }

    if (fadeStarted) {
        float alpha = (fadeTimer / FADE_DURATION) * 255.0f;

        GFX_BeginSceneTop(0, false);
        GFX_DrawRectSolid(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(0, 0, 0, (uint8_t)alpha));

        if (slider != 0.0f) {
            GFX_BeginSceneTop(1, false);
            GFX_DrawRectSolid(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(0, 0, 0, (uint8_t)alpha));
            GFX_BeginSceneTop(0, false);
        }
    }

    GFX_BeginSceneBottom();

    if (flashTimer > 0.0f) {
        float alpha = (flashTimer / FLASH_DURATION) * 255.0f;
        GFX_DrawRectSolid(0, 0, 1.0f, 320, 240, GFX_COLOR_RGBA(255, 255, 255, (uint8_t)alpha));
    }
}

void sceneIntroExit(void) {
    unloadCwavIndex(2);
}