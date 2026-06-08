#include <3ds.h>
#include <citro2d.h>
#include "scene_home.h"
#include "inpost_api.h"
#include "scene_manager.h"
#include "main.h"
#include "cwav_shit.h"
#include "sprites.h"
#include "text.h"
#include <stdio.h>
#include "scene_debug.h"
#include "scene_init.h"
#include <math.h>
#include "qrcodegen.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scene_mainmenu.h"
#include "drawing.h"
#include "cJSON.h"
#include "utils.h"

extern int paczka_count;
extern Paczka paczka_list[];

static int formatIsoDateTime(const char* iso, char* out, size_t outsz) {
    if (!iso || !*iso || !out || outsz == 0) return 0;

    int y = 0, m = 0, d = 0, hh = 0, mm = 0, ss = 0, ms = 0;
    int n = sscanf(iso, "%4d-%2d-%2dT%2d:%2d:%2d.%3d", &y, &m, &d, &hh, &mm, &ss, &ms);
    if (n < 6) n = sscanf(iso, "%4d-%2d-%2dT%2d:%2d:%2d", &y, &m, &d, &hh, &mm, &ss);
    if (n < 5) n = sscanf(iso, "%4d-%2d-%2dT%2d:%2d", &y, &m, &d, &hh, &mm);
    if (n < 5) return 0;

    // hack żeby apka pokazywała poprawny czas: hardcoded europe/warsaw
    struct tm time = {0};
    int isDst = 0;

    time.tm_sec = ss;
    time.tm_min = mm;
    time.tm_hour = hh;
    time.tm_mday = d;
    time.tm_mon = m - 1;
    time.tm_year = y - 1900;

    mktime(&time); // żeby przeliczyło dzień tygodnia

    if (time.tm_mon > 2 && time.tm_mon < 9) isDst = 1;

    if (time.tm_mon == 2) {
        if ((time.tm_mday - time.tm_wday) >= 25) isDst = 1;
    }
    if (time.tm_mon == 9) {
        if ((time.tm_mday - time.tm_wday) < 25) isDst = 1;
    }

    if (isDst) {
        time.tm_hour += 2;
    } else {
        time.tm_hour += 1;
    }

    int written = snprintf(out, outsz, "%02d.%02d.%04d | %02d:%02d", time.tm_mday, time.tm_mon + 1, time.tm_year + 1900, time.tm_hour, time.tm_min);
    return (written > 0 && (size_t)written < outsz) ? 1 : 0;
}

static u64 lastTick = 0;
static float dt = 0.0f;
static float dtScale = 1.0f;

float scroll_speed2 = 0.5f;
float bg_x2 = 0.0f;
float bottomCameraX = 0.0f;
float bottomCameraTargetX = 0.0f;
bool flashActive2 = false;
float flash_timer2 = 0.0f;
float fadeTimer2 = 0.0f;
float how_much_in_flash2 = 1.0f;
float FADE_DURATION2 = 0.6f;
bool tryToGetPaczkas;
bool tryToGetPersonalData;
bool paczkasparsed;
bool dissapeared2 = false;
#define SPINNER_DOT_COUNT 12
float spinnerAngle = 0.0f;
float spinnerRadius = 20.0f;
float spinnerSpeed = 180.0f;
C2D_ImageTint dotTint;
C2D_ImageTint shadowTint; 
float pkgAnimTimer = 0.0f;
int selectedPaczkaIndex = 0;


GFX_TEXTBUF detailsTextBuf; 
GFX_TEXTBUF eventListBuf;   

GFX_TEXT text_name, text_addr, text_status, date_addr, text_event_line; 

int pressedBtnId = 0; 

bool is_in_open_paczkomat_flow = false;
bool inDetailsMode = false;
bool showingQrMode = false; 
bool showOpenConfirmation = false;
bool showPickupConfirmation = false;
bool waitingForOpenCompletion = false;


float eventListScroll = 0.0f;
float eventListTargetScroll = 0.0f;
int touchLastY = 0;
bool isDraggingEvents = false;


bool inSettingsMode = false;

void openCredits(void) {
    sceneManagerSwitchTo(SCENE_CREDITS);
}

static int initialVoiceAct = 0;


bool finishingOpenAnim = false;
float openingSpinnerAlpha = 0.0f;


float popupAnimTimer = 0.0f;
float POPUP_ANIM_DURATION = 0.4f;

float transitionTimer = 0.0f;
float TRANSITION_DURATION = 1.6f;

static C3D_Tex qrCodeTex;
static C2D_Image qrCodeImg;
static Tex3DS_SubTexture qrSubTex;
static int lastGeneratedQrIndex = -1;
static bool qrTexInitialized = false;


static int cachedTextPaczkaIndex = -1;


float easeOutCubic(float t) {
    float f = 1.0f - t;
    return 1.0f - (f * f * f);
}

float easeOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float f = t - 1.0f;
    return 1.0f + c3 * (f * f * f) + c1 * (f * f);
}

void swizzleTexture(uint8_t* dst, const uint8_t* src, int width, int height) {
    int rowblocks = (width >> 3);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            u32 swizzledIndex = ((((y >> 3) * rowblocks + (x >> 3)) << 6) +
                                ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) |
                                ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3)));
            dst[swizzledIndex * 4]     = src[(y * width + x) * 4];
            dst[swizzledIndex * 4 + 1] = src[(y * width + x) * 4 + 1];
            dst[swizzledIndex * 4 + 2] = src[(y * width + x) * 4 + 2];
            dst[swizzledIndex * 4 + 3] = src[(y * width + x) * 4 + 3];
        }
    }
}
void drawNoPaczkasOverlay(GFX_TEXT* textObj, float x, float y) {

    if (paczka_count == 0) {
        GFX_DrawShadowedText(
            textObj,
            x,
            y,
            0.6f,
            0.7f,
            0.7f, GFX_ALIGN_CENTER,
            GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff),
            GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff)
        );
    }
}
void updateQrCodeTexture(const char* qrData) {
    if (!qrData || strlen(qrData) == 0) return;

    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

    bool ok = qrcodegen_encodeText(qrData, tempBuffer, qrcode, qrcodegen_Ecc_LOW,
                                   qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
                                   qrcodegen_Mask_AUTO, true);
    if (!ok) return;

    int qrSize = qrcodegen_getSize(qrcode);
    int texSize = 128;

    uint8_t* linearBuf = (uint8_t*)linearAlloc(texSize * texSize * 4);
    if (!linearBuf) return;

    memset(linearBuf, 0, texSize * texSize * 4);

    int scale = texSize / qrSize;
    if (scale < 1) scale = 1;

    int startX = (texSize - (qrSize * scale)) / 2;
    int startY = (texSize - (qrSize * scale)) / 2;

    for (int y = 0; y < qrSize; y++) {
        for (int x = 0; x < qrSize; x++) {
            bool isDark = qrcodegen_getModule(qrcode, x, y);

            for (int dy = 0; dy < scale; dy++) {
                for (int dx = 0; dx < scale; dx++) {
                    int px = startX + x * scale + dx;
                    int py = startY + y * scale + dy;

                    if (px < texSize && py < texSize) {
                        int idx = (py * texSize + px) * 4;
                        if (isDark) {
                            linearBuf[idx] = 0xFF;
                            linearBuf[idx+1] = 0x00;
                            linearBuf[idx+2] = 0x00;
                            linearBuf[idx+3] = 0x00;
                        } else {
                            linearBuf[idx] = 0xFF;
                            linearBuf[idx+1] = 0xFF;
                            linearBuf[idx+2] = 0xFF;
                            linearBuf[idx+3] = 0xFF;
                        }
                    }
                }
            }
        }
    }

    swizzleTexture((uint8_t*)qrCodeTex.data, linearBuf, texSize, texSize);
    C3D_TexFlush(&qrCodeTex);
    linearFree(linearBuf);

    qrSubTex.width = texSize;
    qrSubTex.height = texSize;
    qrSubTex.left = 0.0f;
    qrSubTex.top = 1.0f;
    qrSubTex.right = 1.0f;
    qrSubTex.bottom = 0.0f;

    qrCodeImg.tex = &qrCodeTex;
    qrCodeImg.subtex = &qrSubTex;
}

void requestOpenLocker(void) {
    if (selectedPaczkaIndex >= 0 && selectedPaczkaIndex < paczka_count) {
        getPaczkomatStatus(paczka_list[selectedPaczkaIndex].shipmentNumber, paczka_list[selectedPaczkaIndex].opencode, paczka_list[selectedPaczkaIndex].phoneNumber, paczka_list[selectedPaczkaIndex].phonePrefix, paczka_list[selectedPaczkaIndex].latitude, paczka_list[selectedPaczkaIndex].longitude);
    }
}

void onPaczkaCollected(void) {
    cJSON *root = cJSON_Parse((const char*)validate_paczkomat.data);
    if(root) {
        cJSON *uuid_open = cJSON_GetObjectItem(root, "sessionUuid");
        if(uuid_open) {
            terminatePaczka(cJSON_GetStringValue(uuid_open));
        }
        cJSON_Delete(root);
    }
}

void updatePaczkaText(void) {
    if (paczka_count <= 0 || selectedPaczkaIndex < 0 || selectedPaczkaIndex >= paczka_count) return;

    GFX_TextBufClear(detailsTextBuf);
    Paczka *sel = &paczka_list[selectedPaczkaIndex];
    char addressBuffer[150];
    if (sel->courier_paczka) {
        snprintf(addressBuffer, sizeof(addressBuffer), "Paczka Kurierska");
    } else {
        snprintf(addressBuffer, sizeof(addressBuffer), "%s, %s", sel->street, sel->city);
    }

    GFX_TextParse(&text_status, detailsTextBuf, sel->latestEvent);
    GFX_TextParse(&text_name, detailsTextBuf, sel->pickupPointName);
    GFX_TextParse(&text_addr, detailsTextBuf, addressBuffer);
    char storedPretty[32];
    const char* storedToShow = sel->storedDate;
    if (formatIsoDateTime(sel->storedDate, storedPretty, sizeof(storedPretty))) {
        storedToShow = storedPretty;
    }
    GFX_TextParse(&date_addr, detailsTextBuf, storedToShow);

    GFX_TextOptimize(&text_status);
    GFX_TextOptimize(&text_name);
    GFX_TextOptimize(&text_addr);
    GFX_TextOptimize(&date_addr);

    cachedTextPaczkaIndex = selectedPaczkaIndex;
}
int va_offset2 = 0;
GFX_TEXT no_parcels_text;
void sceneHomeMenuInit(void) {
    lastTick = svcGetSystemTick();
    dt = 0.0f;
    dtScale = 1.0f;
    va_offset2 = VOICEACT - 1;
    fadeTimer2 = 0.0f;
    flash_timer2 = 0.0f;
    how_much_in_flash2 = 1.0f;
    FADE_DURATION2 = 0.6f;
    flashActive2 = true;
    C2D_PlainImageTint(&dotTint, GFX_COLOR_RGBA(255, 255, 255, 255), 1.0f);
    C2D_PlainImageTint(&shadowTint, GFX_COLOR_RGBA(0, 0, 0, 80), 1.0f);
    tryToGetPaczkas = false;
    tryToGetPersonalData = false;
    paczkasparsed = false;
    inDetailsMode = false;
    showingQrMode = false;
    showOpenConfirmation = false;
    showPickupConfirmation = false;
    waitingForOpenCompletion = false;
    finishingOpenAnim = false;
    openingSpinnerAlpha = 0.0f;
    popupAnimTimer = 0.0f;
    transitionTimer = 0.0f;
    pkgAnimTimer = 0.0f;
    is_in_open_paczkomat_flow = false;
    selectedPaczkaIndex = 0;
    cachedTextPaczkaIndex = -1;
    eventListScroll = 0.0f;
    eventListTargetScroll = 0.0f;

    if (!musicisplaying) {
        CWAV* title = getCwav(1);
        if(title) {
            title->volume = 0.50f;
            playCwav(1, true);
        }
    }

    inSettingsMode = false;


    detailsTextBuf = GFX_TextBufNew(4096);
    eventListBuf = GFX_TextBufNew(4096);


    GFX_TextParse(&no_parcels_text, detailsTextBuf, "Brak Paczek :(");
    GFX_TextOptimize(&no_parcels_text);

    if (!qrTexInitialized) {
        C3D_TexInit(&qrCodeTex, 128, 128, GPU_RGBA8);
        qrTexInitialized = true;
    }
    lastGeneratedQrIndex = -1;
    if (fake_paczka_mode) {
        tryToGetPaczkas = true;
        tryToGetPersonalData = true;
        paczkas.done = true;
        pers_data_done = true;
    }
}

void sceneHomeMenuUpdate(uint32_t kDown, uint32_t kHeld) {
    u64 currentTick = svcGetSystemTick();
    dt = (float)(currentTick - lastTick) / CPU_TICKS_PER_MSEC / 1000.0f;
    lastTick = currentTick;

    if (dt > 0.1f) dt = 0.1f;
    if (dt < 0.001f) dt = 0.001f;

    dtScale = dt * 60.0f;

    flash_timer2 += dt;
    fadeTimer2 += dt;

    if (showOpenConfirmation || showPickupConfirmation) {
        popupAnimTimer += dt;
        if (popupAnimTimer > POPUP_ANIM_DURATION) popupAnimTimer = POPUP_ANIM_DURATION;
    }

    if (flash_timer2 > how_much_in_flash2 && !dissapeared2) {
        dissapeared2 = true;
        flash_timer2 = 0;
    } else if (flash_timer2 > how_much_in_flash2 && dissapeared2){
        dissapeared2 = false;
        flash_timer2 = 0;
    }

    if (fadeTimer2 >= FADE_DURATION2) {
        flashActive2 = false;
        fadeTimer2 = FADE_DURATION2;
    }

    if (!paczkas.done || waitingForOpenCompletion || finishingOpenAnim){
        spinnerAngle += spinnerSpeed * dt;
        if (spinnerAngle >= 360.0f) spinnerAngle -= 360.0f;
    }

    if (waitingForOpenCompletion) {
        openingSpinnerAlpha += 3.0f * dt;
        if (openingSpinnerAlpha > 1.0f) openingSpinnerAlpha = 1.0f;
    } else if (finishingOpenAnim) {
        openingSpinnerAlpha -= 3.0f * dt;
        if (openingSpinnerAlpha <= 0.0f) {
            openingSpinnerAlpha = 0.0f;
            finishingOpenAnim = false;
            switch(VOICEACT) {
                case 0:
                    break;
                default:
                    playCwav(28 + (va_offset2 * 26), true);
                    break;
            }
            showPickupConfirmation = true;
            popupAnimTimer = 0.0f;
            playCwav(4, true);
        }
    }


    if (inSettingsMode) {
        touchPosition touch;
        hidTouchRead(&touch);


        if (kDown & KEY_LEFT) {
            VOICEACT--;
            if (VOICEACT < 0) VOICEACT = 0;
            va_offset2 = VOICEACT - 1;
            playCwav(3, true);
        }
        if (kDown & KEY_RIGHT) {
            VOICEACT++;
            if (VOICEACT > 3) VOICEACT = 3;
            va_offset2 = VOICEACT - 1;
            playCwav(3, true);
        }


        if (kDown & KEY_TOUCH) {

            if (VOICEACT == initialVoiceAct) {
                if (touch.py >= 110 && touch.py <= 140 && touch.px >= 90 && touch.px <= 230) {
                     inSettingsMode = false;
                     sceneManagerSwitchTo(SCENE_TUTORIAL);
                     playCwav(4, true);
                }
            }


            if (touch.py >= 180 && touch.py <= 210 && touch.px >= 110 && touch.px <= 210) {
                inSettingsMode = false;
                cJSON *oproot = cJSON_CreateObject();

                cJSON_AddItemToObject(oproot, "VA", cJSON_CreateNumber(VOICEACT));

                save_json("/3ds/InPost3DS/opcje.json", oproot);
                cJSON_Delete(oproot);
                stopCwav(1);
                freeAllCwavs();
                sceneManagerSwitchTo(SCENE_MAIN_MENU);
            }
        }

        if (kDown & KEY_A) {
            inSettingsMode = false;
            cJSON *oproot = cJSON_CreateObject();

            cJSON_AddItemToObject(oproot, "VA", cJSON_CreateNumber(VOICEACT));

            save_json("/3ds/InPost3DS/opcje.json", oproot);
            cJSON_Delete(oproot);
            stopCwav(1);
            freeAllCwavs();
            sceneManagerSwitchTo(SCENE_MAIN_MENU);
        }
        if (kDown & KEY_B) {
            inSettingsMode = false;
            playCwav(4, true);
        }
        return;
    }


    if (!tryToGetPaczkas){
        getPaczkas();
        tryToGetPaczkas = true;
    }
    if (!tryToGetPersonalData){
        getPersonalData();
        tryToGetPersonalData = true;
    }
    if (dane_usera.done && !pers_data_done){
        parsePersonalData((const char*)dane_usera.data);
    }
    if (debugmode && pers_data_done && (kDown & KEY_X)){
        sceneManagerSwitchTo(SCENE_DEBUG);
    }
    if (paczkas.done && !paczkasparsed){
        if (fake_paczka_mode)
            parseFakePaczkas();
        else
            parsePaczkas((const char*)paczkas.data);

        paczkasparsed = true;
        updatePaczkaText();
    }

    if (validate_paczkomat.done && is_in_open_paczkomat_flow) {
        cJSON *root = cJSON_Parse((const char*)validate_paczkomat.data);
        if (root) {
            cJSON *uuid_open = cJSON_GetObjectItem(root, "sessionUuid");
            open_paczkomat.done = false;
            if(uuid_open) openPaczkomat(cJSON_GetStringValue(uuid_open));
            cJSON_Delete(root);
        }
        is_in_open_paczkomat_flow = false;
        waitingForOpenCompletion = true;
    }

    if (waitingForOpenCompletion && open_paczkomat.done) {
        waitingForOpenCompletion = false;
        finishingOpenAnim = true;
    }

    if (paczkasparsed) {
        if (selectedPaczkaIndex != cachedTextPaczkaIndex) {
            updatePaczkaText();
        }

        if (showPickupConfirmation) {
            if (popupAnimTimer >= POPUP_ANIM_DURATION * 0.5f) {
                if (kDown & KEY_A) {
                    onPaczkaCollected();
                    tryToGetPaczkas = false;
                    tryToGetPersonalData = false;
                    paczkasparsed = false;
                    showPickupConfirmation = false;
                    inDetailsMode = false;
                    paczkas.done = false;
                    playCwav(4, true);
                }
                if (kDown & KEY_B) {
                    showPickupConfirmation = false;
                }
            }
            return;
        }

        if (showOpenConfirmation) {
            if (popupAnimTimer >= POPUP_ANIM_DURATION * 0.5f) {
                if (kDown & KEY_A) {
                    requestOpenLocker();
                    showOpenConfirmation = false;
                    playCwav(4, true);
                    is_in_open_paczkomat_flow = true;
                }
                if (kDown & KEY_B) {
                    showOpenConfirmation = false;
                    is_in_open_paczkomat_flow = false;
                }
            }
            return;
        }


        touchPosition touch;
        hidTouchRead(&touch);


        if (inDetailsMode) {
            transitionTimer += 2.0f * dt;
            if (transitionTimer > TRANSITION_DURATION) transitionTimer = TRANSITION_DURATION;

            if ((selectedPaczkaIndex != lastGeneratedQrIndex && paczka_count > 0) && !paczka_list[selectedPaczkaIndex].courier_paczka) {
                updateQrCodeTexture(paczka_list[selectedPaczkaIndex].qrCode);
                lastGeneratedQrIndex = selectedPaczkaIndex;
            }

            if (kDown & KEY_B) {
                if (showingQrMode) {
                    showingQrMode = false;
                } else {
                    inDetailsMode = false;
                    is_in_open_paczkomat_flow = false;
                }
                return;
            }

            if (showingQrMode && osGetWifiStrength() > 0) {
                 if ((kDown & KEY_TOUCH) && !paczka_list[selectedPaczkaIndex].courier_paczka) {
                    if (touch.px >= (160 - 90) && touch.px <= (160 + 90) &&
                        touch.py >= (200 - 20) && touch.py <= (200 + 20)) {
                        switch(VOICEACT) {
                            case 0:
                                break;
                            default:
                                playCwav(27 + (va_offset2 * 26), true);
                                break;
                        }
                        showOpenConfirmation = true;
                        popupAnimTimer = 0.0f;
                        playCwav(4, true);
                    }
                }
            } else {
                if (!paczka_list[selectedPaczkaIndex].courier_paczka && paczka_list[selectedPaczkaIndex].paczka_openable) {
                    if (kDown & KEY_TOUCH) {
                        if (touch.px >= 270 && touch.px <= 310 && touch.py >= 10 && touch.py <= 50) {
                            showingQrMode = true;
                            playCwav(4, true);
                        }
                    }
                }

                if (kDown & KEY_TOUCH) {
                    touchLastY = touch.py;
                    isDraggingEvents = true;
                }
                if (kHeld & KEY_TOUCH && isDraggingEvents) {
                    int dy = touch.py - touchLastY;
                    eventListTargetScroll += dy;
                    touchLastY = touch.py;
                } else {
                    isDraggingEvents = false;
                }

                if (kHeld & KEY_UP) eventListTargetScroll += 300.0f * dt;
                if (kHeld & KEY_DOWN) eventListTargetScroll -= 300.0f * dt;

                float contentHeight = paczka_list[selectedPaczkaIndex].eventCount * 45.0f;
                float minScroll = -(contentHeight - 200.0f);
                if (minScroll > 0) minScroll = 0;

                if (eventListTargetScroll > 0.0f) eventListTargetScroll = 0.0f;
                if (eventListTargetScroll < minScroll) eventListTargetScroll = minScroll;

                float dampFactor = 0.2f * dtScale;
                if (dampFactor > 1.0f) dampFactor = 1.0f;
                eventListScroll += (eventListTargetScroll - eventListScroll) * dampFactor;
            }

        } else {

            transitionTimer -= 2.0f * dt;
            if (transitionTimer < 0.0f) transitionTimer = 0.0f;
            pkgAnimTimer += dt;

            float transT = transitionTimer / TRANSITION_DURATION;


            pressedBtnId = 0;


            if (transT < 0.1f) {

                if (kHeld & KEY_TOUCH) {
                    if (touch.px >= 5 && touch.px <= 70 && touch.py >= 5 && touch.py <= 45) pressedBtnId = 1;
                    else if (touch.px >= 125 && touch.px <= 195 && touch.py >= 5 && touch.py <= 45) pressedBtnId = 2;
                    else if (touch.px >= 250 && touch.px <= 315 && touch.py >= 5 && touch.py <= 45) pressedBtnId = 3;
                }


                if (kDown & KEY_TOUCH) {

                    if ((touch.px >= 5 && touch.px <= 70 && touch.py >= 5 && touch.py <= 45) && osGetWifiStrength() > 0) {
                        tryToGetPaczkas = false;
                        paczkas.done = false;
                        paczkasparsed = false;


                        paczka_count = 0;
                        pkgAnimTimer = 0.0f;


                        playCwav(4, true);
                    }


                    if (touch.px >= 125 && touch.px <= 195 && touch.py >= 5 && touch.py <= 45) {
                        playCwav(4, true);
                        stopCwav(1);
                        openCredits();
                    }


                    if (touch.px >= 250 && touch.px <= 315 && touch.py >= 5 && touch.py <= 45) {
                        inSettingsMode = true;
                        initialVoiceAct = VOICEACT;
                        playCwav(4, true);
                    }
                }
            }
        }

        if (!inDetailsMode) {
            if (paczka_count > 0) {
                bool changed = false;
                if (kDown & KEY_LEFT) {
                    selectedPaczkaIndex--;
                    if (selectedPaczkaIndex < 0) selectedPaczkaIndex = 0;
                    playCwav(3, true);
                    changed = true;
                }
                if (kDown & KEY_RIGHT) {
                    selectedPaczkaIndex++;
                    if (selectedPaczkaIndex >= paczka_count) selectedPaczkaIndex = paczka_count - 1;
                    playCwav(3, true);
                    changed = true;
                }

                if (changed) {
                    updatePaczkaText();
                }

                if (kDown & KEY_A) {
                    inDetailsMode = true;
                    showingQrMode = false;
                    eventListScroll = 0.0f;
                    eventListTargetScroll = 0.0f;
                    playCwav(4, true);

                    if (paczka_count > 0) {
                        Paczka *sel = &paczka_list[selectedPaczkaIndex];
                        if (!sel->courier_paczka && strlen(sel->imageUrl) > 0) {
                            getPaczkomatImage(sel->imageUrl);
                        } else {
                            obrazekdone = false;
                        }
                    }
                }

                float spacing = 114.0f;
                float screenCenterX = 160.0f;
                float startX = 25.0f;
                bottomCameraTargetX = (startX + selectedPaczkaIndex * spacing) - screenCenterX;
                float maxCameraX = (paczka_count - 1) * spacing;
                if (bottomCameraTargetX < 0.0f) bottomCameraTargetX = 0.0f;
                if (bottomCameraTargetX > maxCameraX) bottomCameraTargetX = maxCameraX;

                float dampFactor = 0.15f * dtScale;
                if (dampFactor > 1.0f) dampFactor = 1.0f;
                bottomCameraX += (bottomCameraTargetX - bottomCameraX) * dampFactor;
            }
        }
    }
}

void drawSpinner(float cx, float cy, float alphaVal) {
    if (alphaVal <= 0.0f) return;
    float bigRadius = 40.0f;
    float dotSize = 8.0f;
    float shadowOffset = 2.0f;
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

void drawHomeSceneContent(float offset, float transT, float finalZY) {

    GFX_DrawImageAt(bg_top, bg_x2 - 560 + offset, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
    GFX_DrawImageAt(bg_top, bg_x2 + offset, 0.0f, 0.0f, NULL, 1.0f, 1.0f);


    if (paczkasparsed) {
        if (finalZY > -230.0f) {
            GFX_DrawImageAt(znaczek, 4.0f, finalZY + 4.0f, 0.5f, &shadowTint, 1.0f, 1.0f);
        }
        GFX_DrawImageAt(znaczek, 0.0f, finalZY, 0.5f, NULL, 1.0f, 1.0f);

        if (paczka_count > 0 && finalZY > -230.0f) {
            float centerScreenX = 200.0f;
            float stampVisualCenterY = finalZY + 120.0f;
            float scaleStatus = 0.5f;
            float scaleName = 0.65f;
            float scaleAddr = 0.5f;

            u8 textAlpha = (u8)(255.0f * (1.0f - (transT * 1.2f)));
            if (textAlpha > 0) {
                u32 textColor = GFX_COLOR_RGBA(0, 0, 0, textAlpha);

                float wD, hD;
                GFX_TextGetDimensions(&text_status, scaleStatus, scaleStatus, &wD, &hD);
                GFX_DrawText(&text_status, centerScreenX - (wD / 2.0f), stampVisualCenterY - 35.0f, 0.6f, scaleStatus, scaleStatus, GFX_ALIGN_LEFT, textColor);

                float wS, hS;
                GFX_TextGetDimensions(&date_addr, scaleStatus, scaleStatus, &wS, &hS);
                GFX_DrawText(&date_addr, centerScreenX - (wS / 2.0f), stampVisualCenterY - 65.0f, 0.6f, scaleStatus, scaleStatus, GFX_ALIGN_LEFT, textColor);

                float wN, hN;
                GFX_TextGetDimensions(&text_name, scaleName, scaleName, &wN, &hN);
                GFX_DrawText(&text_name, centerScreenX - (wN / 2.0f), stampVisualCenterY - 5.0f, 0.6f, scaleName, scaleName, GFX_ALIGN_LEFT, textColor);

                float wA, hA;
                GFX_TextGetDimensions(&text_addr, scaleAddr, scaleAddr, &wA, &hA);
                GFX_DrawText(&text_addr, centerScreenX - (wA / 2.0f), stampVisualCenterY + 25.0f, 0.6f, scaleAddr, scaleAddr, GFX_ALIGN_LEFT, textColor);
            }
        } else {

            if (finalZY > -230.0f) {
                 drawNoPaczkasOverlay(&no_parcels_text, 200.0f, finalZY + 110.0f);
            }
        }

        if (transT > 0.0f) {
            float t = fminf(transT * 1.2f, 1.0f);
            float ease = easeOutCubic(t);
            float centerX = 200.0f;
            float baseY   = 120.0f;
            float slideX  = 40.0f;
            float historyX = centerX - (showingQrMode ? slideX * ease : slideX * (1.0f - ease));
            float historyAlphaVal = showingQrMode ? (1.0f - ease) : ease;

            if (historyAlphaVal < 0.0f) historyAlphaVal = 0.0f;
            if (historyAlphaVal > 1.0f) historyAlphaVal = 1.0f;

            if (obrazekdone && !paczka_list[selectedPaczkaIndex].courier_paczka && kuponkurwa.subtex) {
                float actualW = (float)kuponkurwa.subtex->width;
                float actualH = (float)kuponkurwa.subtex->height;
                if (actualW < 1.0f) actualW = 1.0f;
                if (actualH < 1.0f) actualH = 1.0f;
                float maxW = 360.0f;
                float maxH = 200.0f;
                float scaleX = maxW / actualW;
                float scaleY = maxH / actualH;
                float finalScale = (scaleX < scaleY) ? scaleX : scaleY;
                if (finalScale > 1.0f) finalScale = 1.0f;
                float drawW = actualW * finalScale;
                float drawH = actualH * finalScale;
                float drawX = historyX - (drawW / 2.0f);
                float drawY = 120.0f - (drawH / 2.0f);

                C2D_ImageTint imgT;
                C2D_AlphaImageTint(&imgT, historyAlphaVal);
                C2D_ImageTint shadowT;
                C2D_PlainImageTint(&shadowT, GFX_COLOR_RGBA(0, 0, 0, (u8)(100 * historyAlphaVal)), 1.0f);
                C2D_DrawImageAt(kuponkurwa, drawX + 3.0f + offset, drawY + 3.0f, 0.7f, &shadowT, finalScale, finalScale);
                C2D_DrawImageAt(kuponkurwa, drawX + offset, drawY, 0.71f, &imgT, finalScale, finalScale);
            } else {
                GFX_TEXT historyTxt;
                GFX_TextBufClear(eventListBuf);
                GFX_TextParse(&historyTxt, eventListBuf, "Historia Zdarzeń");
                GFX_TextOptimize(&historyTxt);
                GFX_DrawShadowedText(
                    &historyTxt,
                    historyX + offset,
                    baseY,
                    0.7f,
                    1.0f,
                    1.0f, GFX_ALIGN_CENTER,
                    GFX_COLOR_RGBA(255,255,255,(u8)(255 * historyAlphaVal)),
                    GFX_COLOR_RGBA(0,0,0,(u8)(120 * historyAlphaVal))
                );
            }
        }
    }


    if (showOpenConfirmation || showPickupConfirmation) {
        float pT = popupAnimTimer / POPUP_ANIM_DURATION;
        if (pT > 1.0f) pT = 1.0f;
        float alphaFactor = pT;
        u8 dimAlpha = (u8)(150.0f * alphaFactor);
        GFX_DrawRectSolid(0, 0, 0.96f, 400, 240, GFX_COLOR_RGBA(0, 0, 0, dimAlpha));
    }


    if (flashActive2) {
        float fadeProgress = fadeTimer2 / FADE_DURATION2;
        if (fadeProgress > 1.0f) fadeProgress = 1.0f;
        float fadeValue = 1.0f - fadeProgress;
        uint8_t alpha = (uint8_t)(fadeValue * 255.0f);
        GFX_DrawRectSolid(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255, 255, 255, alpha));
    }
}
void drawFancyButton(float x, float y, float w, float h, const char* text, bool isPressed, float alpha) {
    if (alpha <= 0.0f) return;


    float scale = isPressed ? 0.95f : 1.0f;


    float centerX = x + (w / 2.0f);
    float centerY = y + (h / 2.0f);
    float scaledW = w * scale;
    float scaledH = h * scale;
    float finalX = centerX - (scaledW / 2.0f);
    float finalY = centerY - (scaledH / 2.0f);

    u8 a = (u8)(255 * alpha);
    u8 shadowA = (u8)(100 * alpha);


    if (!isPressed) {
        GFX_DrawRectSolid(finalX + 2.0f, finalY + 2.0f, 0.9f, scaledW, scaledH, GFX_COLOR_RGBA(0, 0, 0, shadowA));
    }


    u32 borderColor = GFX_COLOR_RGBA(255, 204, 0, a);
    GFX_DrawRectSolid(finalX - 1.0f, finalY - 1.0f, 0.9f, scaledW + 2.0f, scaledH + 2.0f, borderColor);


    u32 bgColor = isPressed ? GFX_COLOR_RGBA(234, 234, 234, a) : GFX_COLOR_RGBA(254, 254, 254, a);
    GFX_DrawRectSolid(finalX, finalY, 0.9f, scaledW, scaledH, bgColor);


    GFX_TextBufClear(eventListBuf);
    GFX_TEXT btnTxt;
    GFX_TextParse(&btnTxt, eventListBuf, text);
    GFX_TextOptimize(&btnTxt);


    float txtW, txtH;
    GFX_TextGetDimensions(&btnTxt, 0.5f, 0.5f, &txtW, &txtH);
    u32 txtColor = GFX_COLOR_RGBA(205, 154, 0, a);

    GFX_DrawText(&btnTxt, centerX - (txtW / 2.0f), centerY - (txtH / 2.0f), 0.95f, 0.5f, 0.5f, GFX_ALIGN_LEFT, txtColor);
}
void sceneHomeMenuRender(void) {

    bg_x2 -= scroll_speed2 * dtScale;
    if (bg_x2 <= -80.0f) bg_x2 += 80.0f;


    float transT = transitionTimer / TRANSITION_DURATION;
    if (transT > 1.0f) transT = 1.0f;
    if (transT < 0.0f) transT = 0.0f;

    float zAnimDuration = 0.8f;
    float zStartY = -240.0f;
    float zTargetY = 0.0f;
    float currentZY = zStartY;

    if (pkgAnimTimer > 0.0f) {
            float t = pkgAnimTimer / zAnimDuration;
            if (t > 1.0f) t = 1.0f;
            float ease = easeOutCubic(t);
            currentZY = zStartY - ((zStartY - zTargetY) * ease);
    }

    float flyUpOffset = 0.0f;
    if (transT > 0.0f) {
        float upEase = transT * transT * transT;
        flyUpOffset = -280.0f * upEase;
    }

    float finalZY = currentZY + flyUpOffset;


    GFX_BeginSceneTop(0, true);
    
    drawHomeSceneContent(0.0f, transT, finalZY);

    if (slider > 0.0f) {
        GFX_BeginSceneTop(1, true);
        
        drawHomeSceneContent(slider * 5.0f, transT, finalZY);
    }


    GFX_BeginSceneBottom();
    
    GFX_DrawImageAt(bg_bottom, bg_x2, 0.0f, 0.0f, NULL, 1.0f, 1.0f);

    if (!paczkas.done) {
        drawSpinner(160.0f, 120.0f, 1.0f);
    }


    if (!inDetailsMode && transT < 0.1f) {


        float btnAnimT = pkgAnimTimer / 0.5f;
        if (btnAnimT > 1.0f) btnAnimT = 1.0f;
        float ease = easeOutCubic(btnAnimT);


        float yOffset = 20.0f * (1.0f - ease);
        float btnAlpha = ease;


        drawFancyButton(5.0f, 5.0f + yOffset, 65.0f, 40.0f, "Odśwież", (pressedBtnId == 1), btnAlpha);


        drawFancyButton(125.0f, 5.0f + yOffset, 70.0f, 40.0f, "Credits", (pressedBtnId == 2), btnAlpha);


        drawFancyButton(250.0f, 5.0f + yOffset, 65.0f, 40.0f, "Ustawienia", (pressedBtnId == 3), btnAlpha);
    }
    if (paczkasparsed && paczka_count > 0) {
        float listAlpha = 1.0f;
        if (transT > 0.0f) listAlpha = 1.0f - transT;

        if (listAlpha > 0.05f) {
            float spacing = 114.0f;
            float startX = 25.0f;
            float entryAnimDuration = 0.6f;
            float entryStaggerDelay = 0.15f;
            float startY = 260.0f;
            float targetY = 80.0f;

            for (int i = 0; i < paczka_count; i++) {
                float currentY = startY;
                float myTime = pkgAnimTimer - (i * entryStaggerDelay);
                float yPos = startY;
                float alpha = 0.0f;

                if (myTime > 0.0f) {
                    float t = myTime / entryAnimDuration;
                    if (t > 1.0f) t = 1.0f;
                    float baseEase = easeOutCubic(t);
                    yPos = startY - ((startY - targetY) * baseEase);
                    alpha = baseEase;
                }

                float flyDownOffset = 0.0f;
                if (transT > 0.0f) {
                     float ease = transT * transT * transT;
                     flyDownOffset = 200.0f * ease;
                     alpha *= (1.0f - ease);
                }
                currentY = yPos + flyDownOffset;

                if (currentY < 260.0f && alpha > 0.0f) {
                    float drawX = startX + (i * spacing) - bottomCameraX;
                    C2D_ImageTint pkgTint;
                    C2D_AlphaImageTint(&pkgTint, alpha);
                    C2D_ImageTint shdwTint;
                    C2D_PlainImageTint(&shdwTint, GFX_COLOR_RGBA(0, 0, 0, (u8)(alpha * 80.0f)), 1.0f);

                    GFX_DrawImageAt(paczka_closed, drawX + 5.0f, currentY + 5.0f, 0.5f, &shdwTint, 1.0f, 1.0f);
                    GFX_DrawImageAt(paczka_closed, drawX, currentY, 0.5f, &pkgTint, 1.0f, 1.0f);
                    if (i == selectedPaczkaIndex && !inDetailsMode) {
                        GFX_DrawImageAt(paczka_sel, drawX - 17.0f, currentY - 17.0f, 0.4f, &pkgTint, 1.0f, 1.0f);
                    }
                }
            }
        }
    }

    if (transT > 0.0f) {
        float fInv = 1.0f - transT;
        float detailsEnterEase = 1.0f - (fInv * fInv * fInv);
        float detailsYOffset = 240.0f * (1.0f - detailsEnterEase);

        Paczka *p = &paczka_list[selectedPaczkaIndex];

        if (showingQrMode) {
            float qrStartY = 240.0f;
            float qrTargetY = 56.0f;
            float currentQrY = qrStartY - ((qrStartY - qrTargetY) * detailsEnterEase);

            u8 qrAlpha = (u8)(255.0f * detailsEnterEase);
            u32 qrTintVal = GFX_COLOR_RGBA(255, 255, 255, qrAlpha);
            u32 qrTintVal2 = GFX_COLOR_RGBA(0, 0, 0, 120);

            float cardSize = 138.0f;
            float cardX = 160.0f - (cardSize / 2.0f);

            if (!p->courier_paczka && osGetWifiStrength() > 0) {
                GFX_DrawRectSolid(cardX + 5.0f, currentQrY - 17.50f, 0.8f, cardSize, cardSize, qrTintVal2);
                GFX_DrawRectSolid(cardX, currentQrY - 25.0f, 0.8f, cardSize, cardSize, qrTintVal);

                if (qrTexInitialized) {
                    float qrX = 160.0f - (128.0f / 2.0f);
                    C2D_DrawImageAt(qrCodeImg, qrX, currentQrY - 20.0f, 0.9f, NULL, 1.0f, 1.0f);
                }

                float btnTargetY = 200.0f;
                float btnStartY = 280.0f;
                float currentBtnY = btnStartY - ((btnStartY - btnTargetY) * detailsEnterEase);

                float imgW = otworz_zdalnie_button->width;
                float imgH = otworz_zdalnie_button->height;
                float drawX = 160.0f - (imgW / 2.0f);
                float drawY = currentBtnY - (imgH / 2.0f);

                u8 shadowAlpha = (u8)(100.0f * detailsEnterEase);
                C2D_ImageTint shadowTint;
                C2D_PlainImageTint(&shadowTint, GFX_COLOR_RGBA(0, 0, 0, shadowAlpha), 1.0f);
                GFX_DrawImageAt(otworz_zdalnie_button, drawX + 4.0f, drawY + 4.0f, 0.89f, &shadowTint, 1.0f, 1.0f);
                C2D_ImageTint btnTint;
                C2D_AlphaImageTint(&btnTint, detailsEnterEase);
                GFX_DrawImageAt(otworz_zdalnie_button, drawX, drawY, 0.9f, &btnTint, 1.0f, 1.0f);
            }

        } else {
            u8 listAlpha = (u8)(255.0f * detailsEnterEase);
            float scrollBaseY = detailsYOffset + eventListScroll;
            u32 gradTop = GFX_COLOR_RGBA(255, 255, 255, listAlpha);
            u32 gradBot = GFX_COLOR_RGBA(255, 255, 140, listAlpha);
            GFX_DrawRectangle(0, detailsYOffset, 0.6f, 320, 240, gradTop, gradTop, gradBot, gradBot);

            u32 dateColor = GFX_COLOR_RGBA(100, 100, 100, listAlpha);
            u32 nameColor = GFX_COLOR_RGBA(0, 0, 0, listAlpha);

            GFX_TextBufClear(eventListBuf);

            float listStartOffset = 10.0f;

            if (p->paczka_openable) {
                listStartOffset = 70.0f;
                float fade = 1.0f;
                if (scrollBaseY < detailsYOffset) {
                    fade = fmaxf(0.0f, 1.0f - (detailsYOffset - scrollBaseY) / 40.0f);
                }
                u8 codeAlpha = (u8)(listAlpha * fade);
                GFX_DrawRectSolid(0.0f, scrollBaseY, 0.61f, 320.0f, 50.0f, GFX_COLOR_RGBA(255, 255, 255, (u8)(180 * fade)));

                GFX_TEXT labelTxt, codeTxt;
                GFX_TextParse(&labelTxt, eventListBuf, "Kod Odbioru:");
                GFX_TextOptimize(&labelTxt);
                GFX_DrawText(&labelTxt, 15.0f, scrollBaseY + 12.0f, 0.7f, 0.5f, 0.5f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(100, 100, 100, codeAlpha));

                GFX_TextParse(&codeTxt, eventListBuf, p->opencode);
                GFX_TextOptimize(&codeTxt);
                GFX_DrawText(&codeTxt, 15.0f, scrollBaseY + 30.0f, 0.7f, 0.7f, 0.7f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, codeAlpha));
                GFX_DrawRectSolid(10.0f, scrollBaseY + 55.0f, 0.7f, 300.0f, 2.0f, GFX_COLOR_RGBA(255, 204, 0, codeAlpha));
            }

            float listY = scrollBaseY + listStartOffset;

            for (int i = 0; i < p->eventCount; i++) {
                float itemY = listY + (i * 45.0f);
                if (itemY < detailsYOffset - 40.0f || itemY > 250.0f) continue;

                GFX_TextBufClear(eventListBuf);

                GFX_TEXT dateTxt, nameTxt;

                char eventPretty[32];
                const char* eventToShow = p->events[i].date;
                if (formatIsoDateTime(p->events[i].date, eventPretty, sizeof(eventPretty))) {
                    eventToShow = eventPretty;
                }
                GFX_TextParse(&dateTxt, eventListBuf, eventToShow);
                GFX_TextOptimize(&dateTxt);
                GFX_DrawText(&dateTxt, 10.0f, itemY, 0.7f, 0.45f, 0.45f, GFX_ALIGN_LEFT, dateColor);

                GFX_TextParse(&nameTxt, eventListBuf, p->events[i].name);
                GFX_TextOptimize(&nameTxt);
                GFX_DrawText(&nameTxt, 10.0f, itemY + 15.0f, 0.7f, 0.5f, 0.5f, GFX_ALIGN_LEFT, nameColor);

                GFX_DrawRectSolid(10.0f, itemY + 40.0f, 0.7f, 300.0f, 1.0f, GFX_COLOR_RGBA(200, 200, 200, listAlpha));
            }

            if (!p->courier_paczka && p->paczka_openable) {
                float btnX = 270.0f;
                float btnY = detailsYOffset + 10.0f;
                GFX_DrawRectSolid(btnX, btnY, 0.8f, 40.0f, 40.0f, GFX_COLOR_RGBA(0xFF, 0xAA, 0x00, listAlpha));
                GFX_TEXT qrTxt;
                GFX_TextParse(&qrTxt, eventListBuf, "QR");
                GFX_TextOptimize(&qrTxt);
                GFX_DrawText(&qrTxt, btnX + 8.0f, btnY + 10.0f, 0.9f, 0.6f, 0.6f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(255, 255, 255, listAlpha));
            }
        }
    }

    if (showOpenConfirmation || showPickupConfirmation) {
        float pT = popupAnimTimer / POPUP_ANIM_DURATION;
        if (pT > 1.0f) pT = 1.0f;
        float ease = easeOutBack(pT);
        float alphaFactor = pT;

        u8 dimAlpha = (u8)(150.0f * alphaFactor);
        GFX_DrawRectSolid(0, 0, 0.96f, 320, 240, GFX_COLOR_RGBA(0, 0, 0, dimAlpha));
        
        float mwW = 260.0f;
        float mwH = 100.0f;
        
        float curW = mwW * ease;
        float curH = mwH * ease;
        float curX = (320.0f - curW) / 2.0f;
        float curY = (240.0f - curH) / 2.0f;

        u8 bgAlpha = (u8)(255.0f * alphaFactor);
        GFX_DrawRectSolid(curX, curY, 0.97f, curW, curH, GFX_COLOR_RGBA(255, 255, 255, bgAlpha));
        
        GFX_TEXT qText, optText;
        GFX_TextBufClear(eventListBuf); 

        if (showOpenConfirmation) {
            GFX_TextParse(&qText, eventListBuf, "Czy chcesz otworzyć paczkomat?");
        } else {
            GFX_TextParse(&qText, eventListBuf, "Czy paczka została odebrana?");
        }
        GFX_TextParse(&optText, eventListBuf, "(A) Tak   (B) Nie");
        
        GFX_TextOptimize(&qText);
        GFX_TextOptimize(&optText);
        
        float tScale = 0.5f * ease;
        if (tScale < 0) tScale = 0;

        float wQ, hQ, wO, hO;
        GFX_TextGetDimensions(&qText, tScale, tScale, &wQ, &hQ);
        GFX_TextGetDimensions(&optText, tScale, tScale, &wO, &hO);
        
        u32 txtColor = GFX_COLOR_RGBA(0, 0, 0, bgAlpha);
        GFX_DrawText(&qText, 160.0f - (wQ/2), curY + (20.0f * ease), 0.98f, tScale, tScale, GFX_ALIGN_LEFT, txtColor);
        GFX_DrawText(&optText, 160.0f - (wO/2), curY + (60.0f * ease), 0.98f, tScale, tScale, GFX_ALIGN_LEFT, txtColor);
    }

    if (inSettingsMode) {
        
        GFX_DrawRectSolid(0, 0, 0.99f, 320, 240, GFX_COLOR_RGBA(0, 0, 0, 180));
        

        float mwW = 280.0f;
        float mwH = 220.0f;
        float curX = (320.0f - mwW) / 2.0f;
        float curY = (240.0f - mwH) / 2.0f; 
        
        GFX_DrawRectSolid(curX, curY, 0.995f, mwW, mwH, GFX_COLOR_RGBA(255, 255, 255, 255));
        
        
        GFX_TextBufClear(eventListBuf);
        GFX_TEXT titleTxt, valTxt, okTxt, restartMsg, tutTxt, dpadTxt; 
        GFX_TextParse(&titleTxt, eventListBuf, "Ustawienia");
        GFX_TextOptimize(&titleTxt);
        GFX_DrawText(&titleTxt, 160.0f - 40.0f, curY + 15.0f, 1.0f, 0.7f, 0.7f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0,0,0,255));

        
        char vaBuf[64];
        switch(VOICEACT) {
            case 0:
                snprintf(vaBuf, 64, "< Głos: Off >");
                break;
            case 1:
                snprintf(vaBuf, 64, "< Głos: Męski >");
                break;
            case 2:
                snprintf(vaBuf, 64, "< Głos: Damski >");
                break;
            case 3:
                snprintf(vaBuf, 64, "< Głos: Niebinarny >");
                break;
            default:
                snprintf(vaBuf, 64, "< Głos: %d >", VOICEACT);
                break;
        }
        
        GFX_TextParse(&valTxt, eventListBuf, vaBuf);
        GFX_TextOptimize(&valTxt);
        
        float wV, hV;
        GFX_TextGetDimensions(&valTxt, 0.8f, 0.8f, &wV, &hV);
        GFX_DrawText(&valTxt, 160.0f - (wV/2.0f), curY + 45.0f, 1.0f, 0.8f, 0.8f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(50,50,50,255));

        GFX_TextParse(&dpadTxt, eventListBuf, "(D-Pad)");
        GFX_TextOptimize(&dpadTxt);
        float wD, hD;
        GFX_TextGetDimensions(&dpadTxt, 0.5f, 0.5f, &wD, &hD);
        GFX_DrawText(&dpadTxt, 160.0f - (wD/2.0f), curY + 70.0f, 1.0f, 0.5f, 0.5f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(100,100,100,255));


        float tutBtnY = curY + 100.0f;
        bool voiceChanged = (VOICEACT != initialVoiceAct);

        if (voiceChanged) {
            GFX_DrawRectSolid(160.0f - 70.0f, tutBtnY, 1.0f, 140.0f, 30.0f, GFX_COLOR_RGBA(150, 150, 150, 255));
            GFX_TextParse(&tutTxt, eventListBuf, "Samouczek");
            GFX_TextOptimize(&tutTxt);
            float wT, hT;
            GFX_TextGetDimensions(&tutTxt, 0.6f, 0.6f, &wT, &hT);
            GFX_DrawText(&tutTxt, 160.0f - (wT/2.0f), tutBtnY + 5.0f, 1.0f, 0.6f, 0.6f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(100,100,100,255));
        } else {
            GFX_DrawRectSolid(160.0f - 70.0f, tutBtnY, 1.0f, 140.0f, 30.0f, GFX_COLOR_RGBA(200, 200, 200, 255));
            GFX_TextParse(&tutTxt, eventListBuf, "Samouczek");
            GFX_TextOptimize(&tutTxt);
            float wT, hT;
            GFX_TextGetDimensions(&tutTxt, 0.6f, 0.6f, &wT, &hT);
            GFX_DrawText(&tutTxt, 160.0f - (wT/2.0f), tutBtnY + 5.0f, 1.0f, 0.6f, 0.6f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0,0,0,255));
        }

        GFX_TextParse(&restartMsg, eventListBuf, "(Zapisanie zmian spowoduje restart)");
        GFX_TextOptimize(&restartMsg);
        float wM, hM;
        GFX_TextGetDimensions(&restartMsg, 0.5f, 0.5f, &wM, &hM);
        
        GFX_DrawText(&restartMsg, 160.0f - (wM/2.0f), curY + 145.0f, 1.0f, 0.5f, 0.5f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(200,0,0,255));
        

        float okBtnY = curY + 170.0f;
        GFX_DrawRectSolid(160.0f - 50.0f, okBtnY, 1.0f, 100.0f, 30.0f, GFX_COLOR_RGBA(0, 150, 0, 255));
        GFX_TextParse(&okTxt, eventListBuf, "OK (A)");
        GFX_TextOptimize(&okTxt);
        GFX_DrawText(&okTxt, 160.0f - 23.5f, okBtnY + 5.0f, 1.0f, 0.6f, 0.6f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(255,255,255,255));

        GFX_TEXT cancelTxt;
        GFX_TextParse(&cancelTxt, eventListBuf, "B - Anuluj");
        GFX_TextOptimize(&cancelTxt);
        float wC, hC;
        GFX_TextGetDimensions(&cancelTxt, 0.5f, 0.5f, &wC, &hC);
        GFX_DrawText(&cancelTxt, 160.0f - (wC/2.0f), curY + 203.0f, 1.0f, 0.5f, 0.5f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(100,100,100,255));
    }   
    if (openingSpinnerAlpha > 0.0f) {
        u8 spinBgAlpha = (u8)(100.0f * openingSpinnerAlpha);
        GFX_DrawRectSolid(0, 0, 0.985f, 320, 240, GFX_COLOR_RGBA(0, 0, 0, spinBgAlpha));
        drawSpinner(160.0f, 120.0f, openingSpinnerAlpha);
    }

    if (flashActive2) {
        float fadeProgress = fadeTimer2 / FADE_DURATION2;
        if (fadeProgress > 1.0f) fadeProgress = 1.0f;
        float fadeValue = 1.0f - fadeProgress;
        uint8_t alpha = (uint8_t)(fadeValue * 255.0f);
        GFX_DrawRectSolid(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255, 255, 255, alpha));
    }
}

void sceneHomeMenuExit(void) {
    GFX_TextBufDelete(detailsTextBuf);
    GFX_TextBufDelete(eventListBuf);
    
    tryToGetPaczkas = false;
    paczkas.done = false;
    paczkasparsed = false;
    
    
    paczka_count = 0;       
    pkgAnimTimer = 0.0f;      
    
    

    if (qrTexInitialized) {
        C3D_TexDelete(&qrCodeTex);
        qrTexInitialized = false;
    }
    
}