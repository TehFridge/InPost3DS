#include <3ds.h>
#include <citro2d.h>
#include "scene_mainmenu.h"
#include "scene_manager.h"
#include "main.h"
#include "cwav_shit.h"
#include "sprites.h"
#include "text.h"
#include <math.h>
#include <stdio.h>
#include <jansson.h>
#include <unistd.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "request.h"
#include "inpost_api.h"
#include "scene_debug.h"
#include "scene_init.h"
#include "drawing.h"
#include "cJSON.h"
#include "utils.h"
#define MAX_STARS 64

bool musicisplaying = false;
static u64 lastTick = 0;
static float dt = 0.0f;


float easeOutBackMM(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float f = t - 1.0f;
    return 1.0f + c3 * (f * f * f) + c1 * (f * f);
}

float easeOut(float start, float end, float t) {
    float change = end - start;
    float f = 1.0f - t;
    return start + change * (1.0f - (f * f));
}


float bounceScale = 1.2f;
float bounceVelocity = -0.01f;
bool bounceActive;
bool logoactive = false;
GFX_TEXTBUF MainMenu;

GFX_TEXT menu_Text[50];

GFX_TEXTBUF DebugMainText;

GFX_TEXT dmt_Text[50];

float how_much_in_flash;
bool flashActive;
float FADE_DURATION;
float scroll_speed;
bool accelerating = true;
const float base_speed = 1.0f;
const float max_speed = 10.0f;
const float accel_rate = 0.5f; 
const float decel_rate = 0.5f; 
float x_image = 0;
float x_text = 160.0f;
bool Show_PLZ_LOGIN;
static char mybuf[60];
static char mybuf2[60];
static SwkbdState swkbd;
SwkbdButton button = SWKBD_BUTTON_NONE;
bool Sent_NRTEL;
bool Sent_SMSCODE;
bool LoggedIn;



bool showTutorialConfirmation = false;
float tutorialPopupAnimTimer = 0.0f;
float TUTORIAL_POPUP_DURATION = 0.4f;


float logo_timer;
float login_timer;
bool dissapeared;
float flash_timer;
static float fadeTimer = 0.0f;
bool Switching_To_MainScreen;
bool bounceTextActive = false;
float bounceTextTime = 0.0f;
float bounceTextDuration = 0.6f;
float bounceTextScale = 1.0f;


float bg_x;
static float easet = 0.0f;
float bounceTime = 0.0f;
float bounceDuration = 0.6f;
int va_offset = 0;
void sceneMainMenuInit(void) {
    lastTick = svcGetSystemTick();
    dt = 0.0f;
    
	MainMenu = GFX_TextBufNew(2048);
    DebugMainText = GFX_TextBufNew(2048);
    GFX_TextBufClear(MainMenu);  
	GFX_TextParse(&menu_Text[0], MainMenu, "Wciśnij A");
	GFX_TextParse(&menu_Text[1], MainMenu, "Wprowadź Numer Telefonu (+48)");
    
    
    getCwav(1); 
    getCwav(3); 
    getCwav(83);
    va_offset = VOICEACT - 1;
    switch(VOICEACT) {
        case 0:
            break;
        default:
            getCwav(24 + (va_offset * 26));
            getCwav(25 + (va_offset * 26));
            getCwav(26 + (va_offset * 26));
            getCwav(27 + (va_offset * 26));
            getCwav(28 + (va_offset * 26));
            getCwav(29 + (va_offset * 26));
            getCwav(30 + (va_offset * 26));
            break;
    }

	bounceActive = false;
    
    CWAV* title = getCwav(1);
    if(title) {
        title->volume = 0.50f;
        playCwav(1, true);
        musicisplaying = true;
    }

	how_much_in_flash = 1.0f;
	flashActive = false;
	FADE_DURATION = 0.3f;
	scroll_speed = 0.5f;
	Show_PLZ_LOGIN = false;
	Sent_NRTEL = false;
	Sent_SMSCODE = false;
    showTutorialConfirmation = false;
    tutorialPopupAnimTimer = 0.0f;
    
    
    logo_timer = 0.0f;
    login_timer = 0.0f;
    flash_timer = 0.0f;
    fadeTimer = 0.0f;
    Switching_To_MainScreen = false;
    bounceTextActive = false;
    bounceTextTime = 0.0f;
    bg_x = 0.0f;
    easet = 0.0f;
    bounceTime = 0.0f;
    logoactive = false;
    dissapeared = false;
    accelerating = true;

	if (access("/3ds/InPost3DS/in_post_dane.json", F_OK) != 0){
		LoggedIn = false;
	} else {
		LoggedIn = true;
		cJSON *jsonfl = NULL;
		open_json("/3ds/InPost3DS/in_post_dane.json", &jsonfl);
		if(jsonfl) {
            cJSON *refresh = cJSON_GetObjectItem(jsonfl, "refresh");
            cJSON *access = cJSON_GetObjectItem(jsonfl, "access");
            if(refresh && access) {
                refreshToken = strdup(cJSON_GetStringValue(refresh));
                authToken = strdup(cJSON_GetStringValue(access));
            }
            cJSON_Delete(jsonfl);
        }
	}
         
}

void sceneMainMenuUpdate(uint32_t kDown, uint32_t kHeld) {
    u64 currentTick = svcGetSystemTick();
    dt = (float)(currentTick - lastTick) / CPU_TICKS_PER_MSEC / 1000.0f;
    lastTick = currentTick;
    if (dt > 0.1f) dt = 0.1f;

    
    const float ACCEL_TIME = 1.23f; 
    const float DECEL_TIME = 1.23f; 
    const float accel_per_sec = (max_speed - base_speed) / ACCEL_TIME;
    const float decel_per_sec = (max_speed - base_speed) / DECEL_TIME;
    

    if (showTutorialConfirmation) {
        tutorialPopupAnimTimer += dt;
        if (tutorialPopupAnimTimer > TUTORIAL_POPUP_DURATION) tutorialPopupAnimTimer = TUTORIAL_POPUP_DURATION;

        if (tutorialPopupAnimTimer >= TUTORIAL_POPUP_DURATION * 0.5f) {
            if (kDown & KEY_A) {
                playCwav(4, true);
                sceneManagerSwitchTo(SCENE_TUTORIAL);
            }
            if (kDown & KEY_B) {
                sceneManagerSwitchTo(SCENE_HOME_MENU);
            }
        }
        return; 
    }

    logo_timer += dt;
    flash_timer += dt;

    if (kDown & KEY_A && !Show_PLZ_LOGIN) { 
        playCwav(0, true);
        if (!LoggedIn) {
            fadeTimer = 0.0f;
            how_much_in_flash = 0.005f;
            FADE_DURATION = 0.65f;
            flashActive = true;
            Switching_To_MainScreen = true;
            
            
            accelerating = true;
            scroll_speed = base_speed; 
            easet = 0.0f; 
        } else  {
            sceneManagerSwitchTo(SCENE_HOME_MENU);
        }
    }
    if (debugmode && (kDown & KEY_X)){
        sceneManagerSwitchTo(SCENE_DEBUG);
    }
    if (logo_timer > 0.63f && !logoactive) {
        bounceActive = true;
        logoactive = true;
        flashActive = true;
        fadeTimer = 0.0f; 
        bounceTime = 0.0f;
    }

    if (flash_timer > how_much_in_flash && !dissapeared) {
        dissapeared = true;
        flash_timer = 0;
    } else if (flash_timer > how_much_in_flash && dissapeared){
        dissapeared = false;
        flash_timer = 0;
    }

    if (Switching_To_MainScreen) {
        login_timer += dt;
        
        
        if (accelerating) {
            scroll_speed += accel_per_sec * dt;
            if (scroll_speed >= max_speed) {
                scroll_speed = max_speed;
                accelerating = false; 
                if (VOICEACT != 0) {
                    playCwav(29 + (va_offset * 26), true);
                }
            }
        } else {
            scroll_speed -= decel_per_sec * dt;
            if (scroll_speed <= base_speed) {
                scroll_speed = base_speed;
                
                
                if (!Show_PLZ_LOGIN) {
                    Show_PLZ_LOGIN = true;
                    
                }
            }
        }
        

        
        
        
        if (Show_PLZ_LOGIN && easet >= 1.0f && !Sent_NRTEL){
            swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 9);
            swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
            swkbdSetValidation(&swkbd, SWKBD_FIXEDLEN, 0, 0);
            swkbdSetFeatures(&swkbd, SWKBD_FIXED_WIDTH);
            button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
            sendNumerTel(mybuf);
            Sent_NRTEL = true;
            bounceTextActive = true;
            bounceTextTime = 0.0f;
            GFX_TextParse(&menu_Text[1], MainMenu, "Wprowadź Kod SMS");
            if (VOICEACT != 0) {
                playCwav(30 + (va_offset * 26), true);
            }
        }

        if (send_phone_numer.done && Sent_NRTEL && !Sent_SMSCODE){      
            swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 6);
            swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
            swkbdSetValidation(&swkbd, SWKBD_FIXEDLEN, 0, 0);
            swkbdSetFeatures(&swkbd, SWKBD_FIXED_WIDTH);
            button = swkbdInputText(&swkbd, mybuf2, sizeof(mybuf2));
            
            sendKodSMS(mybuf, mybuf2);
            Sent_SMSCODE = true;
        }

        if (send_kod_sms.done && !showTutorialConfirmation) {
            if (access("/3ds/InPost3DS/in_post_dane.json", F_OK) != 0) {
                cJSON *root = cJSON_Parse(send_kod_sms.data);
                if (root) {
                    cJSON *refreshtoken = cJSON_GetObjectItem(root, "refreshToken");
                    cJSON *accesstoken  = cJSON_GetObjectItem(root, "authToken");
                    if (refreshtoken && accesstoken) {
                        const char* refreshtoken2 = cJSON_GetStringValue(refreshtoken);
                        const char* accesstoken2  = cJSON_GetStringValue(accesstoken);
                        refreshToken = strdup(refreshtoken2);
                        authToken    = strdup(accesstoken2);
                        cJSON *oproot = cJSON_CreateObject();
                        cJSON_AddItemToObject(oproot, "refresh", cJSON_CreateString(refreshtoken2));
                        cJSON_AddItemToObject(oproot, "access", cJSON_CreateString(accesstoken2));
                        save_json("/3ds/InPost3DS/in_post_dane.json", oproot);
                        cJSON_Delete(oproot);
                    }
                    cJSON_Delete(root);
                }
                LoggedIn = true; 
                showTutorialConfirmation = true;
                if (VOICEACT != 0) {
                    playCwav(26 + (va_offset * 26), true);
                }
                tutorialPopupAnimTimer = 0.0f;
            }
        }
    }
    GFX_TextBufClear(DebugMainText);  
    #ifdef DEBUG
        if (!logplz) {
            GFX_TextParse(
            &dmt_Text[2],
            DebugMainText,
            "Build date: " __DATE__ " " __TIME__
            " (DEBUG BUILD)");
        } else {
            GFX_TextParse(
            &dmt_Text[2],
            DebugMainText,
            "Build date: " __DATE__ " " __TIME__
            " (DEBUG BUILD) - logs on");            
        }
    #else
        GFX_TextParse(
        &dmt_Text[2],
        DebugMainText,
        "Ver 1.0.4");
    #endif     
}
void drawMainMenuTop(float offset) {
	GFX_DrawImageAt(bg_top, bg_x - 560 + offset, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
    GFX_DrawImageAt(bg_top, bg_x + offset, 0.0f, 0.0f, NULL, 1.0f, 1.0f);

    if (logoactive) {
		float w = logo3ds->width * bounceScale;
		float h = logo3ds->height * bounceScale;
		float curX = x_image;
        
        if (!Switching_To_MainScreen) {
			curX = ((400.0f - w) / 2.0f);
		}

		C2D_ImageTint shadowTint;
		C2D_PlainImageTint(&shadowTint, GFX_COLOR_RGBA(0, 0, 0, 110), 1.0f);

		GFX_DrawImageAt(logo3ds, curX - 4.0f, ((240.0f - h) / 2.0f) + 4.0f, 1.0f, &shadowTint, bounceScale, bounceScale);
		GFX_DrawImageAt(logo3ds, curX, (240.0f - h) / 2.0f, 1.0f, NULL, bounceScale, bounceScale);
	}

    if (flashActive) {
		float fadeProgress = fadeTimer / FADE_DURATION;
		if (fadeProgress > 1.0f) fadeProgress = 1.0f;

		float fadeValue = 1.0f - fadeProgress; 
		uint8_t alpha = (uint8_t)(fadeValue * 255.0f);
		GFX_DrawRectSolid(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255, 255, 255, alpha));
	}

    if (Show_PLZ_LOGIN) {
		float y = easeOut(400.0f, 100.0f, easet);
		GFX_DrawShadowedText(&menu_Text[1], 200.0f - 4 + offset, y + 4, 1.0f, bounceTextScale, bounceTextScale, GFX_ALIGN_CENTER, GFX_COLOR_RGBA(0x00, 0x00, 0x00, 0x2e), GFX_COLOR_RGBA(0x00, 0x00, 0x00, 0x2e));
		GFX_DrawShadowedText(&menu_Text[1], 200.0f + offset, y, 1.0f, bounceTextScale, bounceTextScale, GFX_ALIGN_CENTER, GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff), GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff));
	}
    
    GFX_DrawShadowedText(&dmt_Text[2], 10.0f, 220, 1.0f, 0.5f, 0.5f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff), GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff));

    if (showTutorialConfirmation) {
        float pT = tutorialPopupAnimTimer / TUTORIAL_POPUP_DURATION;
        if (pT > 1.0f) pT = 1.0f;
        u8 dimAlpha = (u8)(150.0f * pT);
        GFX_DrawRectSolid(0, 0, 0.98f, 400, 240, GFX_COLOR_RGBA(0, 0, 0, dimAlpha));
    }
}

void sceneMainMenuRender(void) {
	bg_x -= scroll_speed * (dt * 60.0f);
	if (bg_x <= -80.0f) {
		bg_x += 80.0f;  
	}

	if (bounceActive) {
		bounceTime += dt;  
		if (bounceTime >= bounceDuration) {
			bounceTime = bounceDuration;
			bounceActive = false;
		}

		float t = bounceTime / bounceDuration; 
		float damping = expf(-6.0f * t);        
		bounceScale = 1.0f + 0.1f * sinf(10.0f * t * M_PI) * damping;
	}

    
    if (flashActive) {
        fadeTimer += dt;
        if (fadeTimer >= FADE_DURATION) {
            fadeTimer = FADE_DURATION;
        }
    }

    if (Show_PLZ_LOGIN) {
        if (bounceTextActive) {
            bounceTextTime += dt;
            if (bounceTextTime >= bounceTextDuration) {
                bounceTextTime = bounceTextDuration;
                bounceTextActive = false;
            }
            float t = bounceTextTime / bounceTextDuration;
            float damping = expf(-6.0f * t);
            bounceTextScale = 0.9f + 0.1f * sinf(10.0f * t * M_PI) * damping;
        } else {
            bounceTextScale = 0.9f;
        }
        if (easet < 1.0f) easet += 1.0f * dt; 
    } else {
        easet = 0.0f;
    }

    if (logoactive && Switching_To_MainScreen) {
         x_image -= scroll_speed * (dt * 60.0f);
    }
    
    GFX_BeginSceneTop(0, true);
    
    drawMainMenuTop(0.0f);

    if (slider > 0.0f) {
        GFX_BeginSceneTop(1, true);
        
        drawMainMenuTop(slider * 5.0f);
    }

    GFX_BeginSceneBottom();
	
	GFX_DrawImageAt(bg_bottom, bg_x, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
	
    if (logoactive) {
		if (Switching_To_MainScreen) {
			x_text -= scroll_speed * (dt * 60.0f);
		} else {
			x_text = 160.0f;
		}
		GFX_DrawShadowedText(&menu_Text[0], x_text, 100.0f, 0.5f, 1.5f, 1.5f, GFX_ALIGN_CENTER, GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, dissapeared ? 0xff : 0x00), GFX_COLOR_RGBA(0xff, 0xff, 0xff, dissapeared ? 0xff : 0x00));
	}

    if (showTutorialConfirmation) {
        float pT = tutorialPopupAnimTimer / TUTORIAL_POPUP_DURATION;
        if (pT > 1.0f) pT = 1.0f;
        float ease = easeOutBackMM(pT);
        float alphaFactor = pT;

        u8 dimAlpha = (u8)(150.0f * alphaFactor);
        GFX_DrawRectSolid(0, 0, 0.98f, 320, 240, GFX_COLOR_RGBA(0, 0, 0, dimAlpha));
        
        float mwW = 260.0f;
        float mwH = 100.0f;
        
        float curW = mwW * ease;
        float curH = mwH * ease;
        float curX = (320.0f - curW) / 2.0f; 
        float curY = (240.0f - curH) / 2.0f;

        u8 bgAlpha = (u8)(255.0f * alphaFactor);
        GFX_DrawRectSolid(curX, curY, 0.99f, curW, curH, GFX_COLOR_RGBA(255, 255, 255, bgAlpha));
        
        GFX_TEXTBUF popupBuf = GFX_TextBufNew(512);
        GFX_TEXT qText, optText;

        GFX_TextParse(&qText, popupBuf, "Czy pierwszy raz korzystasz z Inpost3DS?");
        GFX_TextParse(&optText, popupBuf, "(A) Tak    (B) Nie");
        
        GFX_TextOptimize(&qText);
        GFX_TextOptimize(&optText);
        
        float tScale = 0.5f * ease;
        if (tScale < 0) tScale = 0;

        float wQ, hQ, wO, hO;
        GFX_TextGetDimensions(&qText, tScale, tScale, &wQ, &hQ);
        GFX_TextGetDimensions(&optText, tScale, tScale, &wO, &hO);
        
        u32 txtColor = GFX_COLOR_RGBA(0, 0, 0, bgAlpha);
        GFX_DrawText(&qText, 160.0f - (wQ/2), curY + (25.0f * ease), 1.0f, tScale, tScale, GFX_ALIGN_LEFT, txtColor);
        GFX_DrawText(&optText, 160.0f - (wO/2), curY + (65.0f * ease), 1.0f, tScale, tScale, GFX_ALIGN_LEFT, txtColor);
        
        GFX_TextBufDelete(popupBuf);
    }

	if (flashActive) {
		float fadeProgress = fadeTimer / FADE_DURATION;
		if (fadeProgress > 1.0f) fadeProgress = 1.0f;

		float fadeValue = 1.0f - fadeProgress; 
		uint8_t alpha = (uint8_t)(fadeValue * 255.0f);
        GFX_DrawRectSolid(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255, 255, 255, alpha));
	}
}

void sceneMainMenuExit(void) {
    if (MainMenu) {
        GFX_TextBufDelete(MainMenu);
        MainMenu = NULL;
    }
    
    
    
    

    if (send_phone_numer.data) {
        free(send_phone_numer.data);
        send_phone_numer.data = NULL;
    }
    if (send_kod_sms.data) {
        free(send_kod_sms.data);
        send_kod_sms.data = NULL;
    }
}