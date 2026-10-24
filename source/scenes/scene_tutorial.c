#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "scene_tutorial.h"
#include "scene_manager.h"
#include "scene_debug.h"
#include "scene_mainmenu.h"
#include "../main.h"
#include "../utils/sprites.h"
#include "../utils/text.h"
#include "../utils/cwav_shit.h" 
#include "scene_init.h"
#include "../HAL/drawing.h"
#include "../utils/lang.h"
#include "../utils/anim.h"
static u64 lastTick = 0;
static float dt = 0.0f;


static float tut_bg_x = 0.0f;
static float tut_scroll_speed = 0.5f;

static Ease dialogue_ease;
static Ease char_ease;
static Ease pop_ease;


typedef struct {
    const char* text;
    int speakingImageId;   
    int idleImageId;       
    float speechDuration;  
    GFX_IMAGE* topImage;    
    GFX_IMAGE* bottomImage; 
} TutorialStep;


static float speechTimer = 0.0f; 
static int currentDisplayedCharId = 0; 

// leniwe w chuj ale mam 8 dni do releasu
static TutorialStep tutorialScript_arhn[21];
static TutorialStep tutorialScript[21];

static int totalSteps = sizeof(tutorialScript) / sizeof(TutorialStep);
static int currentStep = 0;

static GFX_TEXTBUF tutorialTextBuf;
static GFX_TEXT currentTextObj;

static GFX_TEXTBUF promptBuf;
static GFX_TEXT promptText;


static float flashTimer = 0.0f;
static float FLASH_DURATION = 0.5f;

static float entranceTimer = 0.0f;
static float ENTRANCE_DURATION = 1.2f;

static float charBounceTimer = 0.0f;
static bool isBouncing = false;
static float BOUNCE_SPEED = 12.0f; 

static float topImgTimer = 0.0f;
static float botImgTimer = 0.0f;
static float IMG_ANIM_DURATION = 0.5f; 
int offset = 0;

void sceneTutorialInit(void) {
    lastTick = svcGetSystemTick();
    dt = 0.0f;
    currentStep = 0;
    
    dialogue_ease = initEase(1.2f, EASE_OUT);
    char_ease = initEase(1.2f, EASE_OUT);
    pop_ease =  initEase(0.8f, EASE_OUTBACKMM);
    speechTimer = 0.0f;
    if (VOICEACT != 3) {
        currentDisplayedCharId = tutorialScript[currentStep].speakingImageId;
    } else {
        currentDisplayedCharId = tutorialScript_arhn[currentStep].speakingImageId;
    }
    
    tutorialScript_arhn[0] = (TutorialStep){ lang_strings[48], 0, 0, 3.0f, NULL, NULL };
    tutorialScript_arhn[1] = (TutorialStep){ lang_strings[49], 1, 1, 3.5f, NULL, NULL };
    tutorialScript_arhn[2] = (TutorialStep){ lang_strings[50], 1, 1, 2.0f, tut0_0, tut0_bot0 };
    tutorialScript_arhn[3] = (TutorialStep){ lang_strings[51], 1, 1, 3.0f, tut0_0, tut0_bot0 };
    tutorialScript_arhn[4] = (TutorialStep){ lang_strings[52], 1, 1, 2.0f, tut0_0, tut0_bot0 };
    tutorialScript_arhn[5] = (TutorialStep){ lang_strings[53], 1, 1, 3.0f, tut0_1, tut0_bot0 };
    tutorialScript_arhn[6] = (TutorialStep){ lang_strings[54], 1, 1, 2.5f, tut0_2, tut0_bot0 };
    tutorialScript_arhn[7] = (TutorialStep){ lang_strings[55], 1, 1, 3.0f, tut0_3, tut0_bot0 };
    tutorialScript_arhn[8] = (TutorialStep){ lang_strings[56], 1, 1, 4.0f, tut0_0, tut0_bot1 };
    tutorialScript_arhn[9] = (TutorialStep){ lang_strings[57], 1, 1, 4.0f, tut1_0, tut1_bot0 };
    tutorialScript_arhn[10] = (TutorialStep){ lang_strings[58], 1, 1, 3.0f, tut1_0, tut1_bot0 };
    tutorialScript_arhn[11] = (TutorialStep){ lang_strings[59], 1, 1, 4.0f, tut1_0, tut1_bot1 };
    tutorialScript_arhn[12] = (TutorialStep){ lang_strings[60], 1, 1, 3.0f, NULL, tut2_0 };
    tutorialScript_arhn[13] = (TutorialStep){ lang_strings[61], 1, 1, 5.0f, NULL, tut2_0 };
    tutorialScript_arhn[14] = (TutorialStep){ lang_strings[62], 2, 2, 3.0f, NULL, tut2_0 };
    tutorialScript_arhn[15] = (TutorialStep){ lang_strings[63], 1, 1, 3.0f, NULL, tut2_0 };
    tutorialScript_arhn[16] = (TutorialStep){ lang_strings[64], 1, 1, 4.0f, NULL, tut3_0 };
    tutorialScript_arhn[17] = (TutorialStep){ lang_strings[65], 1, 1, 3.5f, NULL, tut3_0 };
    tutorialScript_arhn[18] = (TutorialStep){ lang_strings[66], 1, 1, 3.0f, NULL, tut3_0 };
    tutorialScript_arhn[19] = (TutorialStep){ lang_strings[67], 1, 1, 2.0f, NULL, NULL };
    tutorialScript_arhn[20] = (TutorialStep){ lang_strings[71], 0, 0, 4.0f, NULL, NULL };

    tutorialScript[0] = (TutorialStep){ lang_strings[48], 0, 2, 3.0f, NULL, NULL };
    tutorialScript[1] = (TutorialStep){ lang_strings[49], 1, 2, 3.5f, NULL, NULL };
    tutorialScript[2] = (TutorialStep){ lang_strings[50], 1, 2, 2.0f, tut0_0, tut0_bot0 };
    tutorialScript[3] = (TutorialStep){ lang_strings[51], 1, 2, 3.0f, tut0_0, tut0_bot0 };
    tutorialScript[4] = (TutorialStep){ lang_strings[52], 1, 2, 2.0f, tut0_0, tut0_bot0 };
    tutorialScript[5] = (TutorialStep){ lang_strings[53], 1, 2, 3.0f, tut0_1, tut0_bot0 };
    tutorialScript[6] = (TutorialStep){ lang_strings[54], 1, 2, 2.5f, tut0_2, tut0_bot0 };
    tutorialScript[7] = (TutorialStep){ lang_strings[55], 1, 2, 3.0f, tut0_3, tut0_bot0 };
    tutorialScript[8] = (TutorialStep){ lang_strings[56], 1, 2, 4.0f, tut0_0, tut0_bot1 };
    tutorialScript[9] = (TutorialStep){ lang_strings[57], 1, 2, 4.0f, tut1_0, tut1_bot0 };
    tutorialScript[10] = (TutorialStep){ lang_strings[58], 1, 2, 3.0f, tut1_0, tut1_bot0 };
    tutorialScript[11] = (TutorialStep){ lang_strings[59], 1, 2, 4.0f, tut1_0, tut1_bot1 };
    tutorialScript[12] = (TutorialStep){ lang_strings[60], 1, 2, 3.0f, NULL, tut2_0 };
    tutorialScript[13] = (TutorialStep){ lang_strings[61], 1, 2, 5.0f, NULL, tut2_0 };
    tutorialScript[14] = (TutorialStep){ lang_strings[62], 3, 4, 3.0f, NULL, tut2_0 };
    tutorialScript[15] = (TutorialStep){ lang_strings[63], 0, 2, 3.0f, NULL, tut2_0 };
    tutorialScript[16] = (TutorialStep){ lang_strings[64], 1, 2, 4.0f, NULL, tut3_0 };
    tutorialScript[17] = (TutorialStep){ lang_strings[65], 1, 2, 3.5f, NULL, tut3_0 };
    tutorialScript[18] = (TutorialStep){ lang_strings[66], 1, 2, 3.0f, NULL, tut3_0 };
    tutorialScript[19] = (TutorialStep){ lang_strings[67], 1, 2, 2.0f, NULL, NULL };
    tutorialScript[20] = (TutorialStep){ lang_strings[71], 0, 2, 4.0f, NULL, NULL };

    flashTimer = 0.0f;
    
    musicisplaying = true;

    entranceTimer = 0.0f;
    charBounceTimer = 0.0f;
    isBouncing = false;
    topImgTimer = 0.0f;
    botImgTimer = 0.0f;
    tut_bg_x = 0.0f;

    tutorialTextBuf = GFX_TextBufNew(4096);
    GFX_TextBufClear(tutorialTextBuf);
    if (VOICEACT != 3) {
        GFX_TextParse(&currentTextObj, tutorialTextBuf, tutorialScript[currentStep].text);
        GFX_TextOptimize(&currentTextObj);
    } else {
         GFX_TextParse(&currentTextObj, tutorialTextBuf, tutorialScript_arhn[currentStep].text);
        GFX_TextOptimize(&currentTextObj);
    }
    


    promptBuf = GFX_TextBufNew(256);
    GFX_TextParse(&promptText, promptBuf, lang_strings[20]);
    GFX_TextOptimize(&promptText);

    switch(VOICEACT) {
        case 1:
            offset = 0;
            for(int i = 0; i < totalSteps; i++) {
                getCwav(5  + i + (26 * offset));
            }
            playCwav(5 + currentStep + (26 * offset), false);
            break;
        case 2:
            offset = 1;
            for(int i = 0; i < totalSteps; i++) {
                getCwav(5  + i + (26 * offset));
            }
            playCwav(5 + currentStep + (26 * offset), false);
            break;
        case 3:
            offset = 2;
            for(int i = 0; i < totalSteps; i++) {
                getCwav(5  + i + (26 * offset));
            }
            playCwav(5 + currentStep + (26 * offset), false);
            break;
        default:

            break;
    }    


   
}

void sceneTutorialUpdate(uint32_t kDown, uint32_t kHeld) {
    u64 currentTick = svcGetSystemTick();
    dt = (float)(currentTick - lastTick) / CPU_TICKS_PER_MSEC / 1000.0f;
    lastTick = currentTick;
    if (dt > 0.1f) dt = 0.1f;
    updateEase(&pop_ease);
    updateEase(&dialogue_ease);
    updateEase(&char_ease);
    
    speechTimer += dt;
    if (VOICEACT != 3) {
        if (currentStep < totalSteps) {
            if (speechTimer >= tutorialScript[currentStep].speechDuration) {
                currentDisplayedCharId = tutorialScript[currentStep].idleImageId;
            }
        }
    } else {
        if (currentStep < totalSteps) {
            if (speechTimer >= tutorialScript_arhn[currentStep].speechDuration) {
                currentDisplayedCharId = tutorialScript_arhn[currentStep].idleImageId;
            }
        }
    }


    

    tut_bg_x -= tut_scroll_speed * (dt * 60.0f);
    if (tut_bg_x <= -80.0f) tut_bg_x += 80.0f;

    if (flashTimer < FLASH_DURATION) {
        flashTimer += dt;
        if (flashTimer > FLASH_DURATION) flashTimer = FLASH_DURATION;
    }
    if (entranceTimer < ENTRANCE_DURATION) {
        entranceTimer += dt;
        if (entranceTimer > ENTRANCE_DURATION) entranceTimer = ENTRANCE_DURATION;
    }
    if (isBouncing) {
        charBounceTimer += BOUNCE_SPEED * dt;
        if (charBounceTimer >= M_PI) { 
            charBounceTimer = 0.0f;
            isBouncing = false;
        }
    }
    if (topImgTimer < IMG_ANIM_DURATION) {
        topImgTimer += dt;
        if (topImgTimer > IMG_ANIM_DURATION) topImgTimer = IMG_ANIM_DURATION;
    }
    if (botImgTimer < IMG_ANIM_DURATION) {
        botImgTimer += dt;
        if (botImgTimer > IMG_ANIM_DURATION) botImgTimer = IMG_ANIM_DURATION;
    }

    if (flashTimer >= FLASH_DURATION * 0.5f) {
        if (kDown & KEY_A) {
            int nextStep = currentStep + 1;
            
            if (nextStep >= totalSteps) {
                sceneManagerSwitchTo(SCENE_HOME_MENU);
            } else {
                GFX_IMAGE* oldTop = tutorialScript[currentStep].topImage;
                GFX_IMAGE* newTop = tutorialScript[nextStep].topImage;
                GFX_IMAGE* oldBot = tutorialScript[currentStep].bottomImage;
                GFX_IMAGE* newBot = tutorialScript[nextStep].bottomImage;

                if (newTop != oldTop && newTop != NULL) topImgTimer = 0.0f;
                if (newBot != oldBot && newBot != NULL) botImgTimer = 0.0f;

                switch(VOICEACT) {
                    case 0:
                        break;
                    default:
                        stopCwav(5  + currentStep + (26 * offset));
                        break;
                }
                // ej wsm dobra, porównywanie pointerów ma sens xD.
                switch(VOICEACT) {
                    case 3:
                        if (tutorialScript_arhn[currentStep].topImage != tutorialScript_arhn[currentStep+1].topImage) {
                            isBouncing = true;
                            pop_ease =  initEase(0.8f, EASE_OUTBACKMM);
                        } else {
                            isBouncing = false;
                        }              
                    default:
                        if (tutorialScript[currentStep].topImage != tutorialScript[currentStep+1].topImage) {
                            isBouncing = true;
                            pop_ease =  initEase(0.8f, EASE_OUTBACKMM);
                        } else {
                            isBouncing = false;
                        }
                }
                currentStep = nextStep;
                speechTimer = 0.0f;
                if (VOICEACT != 3) {
                    currentDisplayedCharId = tutorialScript[currentStep].speakingImageId;
                } else {
                    currentDisplayedCharId = tutorialScript_arhn[currentStep].speakingImageId;
                }
                switch(VOICEACT) {
                    case 0:
                        break;
                    default:
                        playCwav(5 + currentStep + (26 * offset), false);
                        break;
                }

                GFX_TextBufClear(tutorialTextBuf);
                if (VOICEACT != 3) {
                    GFX_TextParse(&currentTextObj, tutorialTextBuf, tutorialScript[currentStep].text);
                } else {
                    GFX_TextParse(&currentTextObj, tutorialTextBuf, tutorialScript_arhn[currentStep].text);
                }
                GFX_TextOptimize(&currentTextObj);
                
                isBouncing = true;
                
                charBounceTimer = 0.0f;
            }
        }
        
        if (kDown & KEY_SELECT) {
             sceneManagerSwitchTo(SCENE_HOME_MENU);
        }
    }
}

void drawTutorialTop(float offset) {
    GFX_DrawImageAt(bg_top, tut_bg_x - 560 + offset, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
    GFX_DrawImageAt(bg_top, tut_bg_x + offset, 0.0f, 0.0f, NULL, 1.0f, 1.0f);

    if (VOICEACT != 3) {
        if (tutorialScript[currentStep].topImage != NULL) {
            GFX_IMAGE* img = tutorialScript[currentStep].topImage;
            float drawW = img->width * pop_ease.value;
            float drawH = img->height * pop_ease.value;

            float x = 200.0f - (drawW / 2.0f);
            float y = 120.0f - (drawH / 2.0f);
            
            GFX_DrawImageAt(img, x + offset, y, 0.5f, NULL, pop_ease.value, pop_ease.value);
        }
    } else {
        if (tutorialScript_arhn[currentStep].topImage != NULL) {
            GFX_IMAGE* img = tutorialScript_arhn[currentStep].topImage;
        

            float drawW = img->width * pop_ease.value;
            float drawH = img->height * pop_ease.value;

            float x = 200.0f - (drawW / 2.0f);
            float y = 120.0f - (drawH / 2.0f);
            
            GFX_DrawImageAt(img, x + offset, y, 0.5f, NULL, pop_ease.value, pop_ease.value);
        } 
    }

    float charBaseX = -180.0f + (200.0f * char_ease.value); 
    float charBaseY = 40.0f;
    
    float bounceY = 0.0f;
    if (isBouncing) {
        bounceY = -10.0f * sinf(charBounceTimer); 
    }
    float idleBob = sinf(osGetTime() / 500.0f) * 2.0f;
    float finalCharY = charBaseY + bounceY + idleBob;
    
    float charW = 100.0f;
    
    float boxH = 80.0f;
    float targetBoxY = 240.0f - boxH;
    float startBoxY = 240.0f; 
    
    float currentBoxY = startBoxY - ((startBoxY - targetBoxY) * dialogue_ease.value);
    
    u32 colorTop = GFX_COLOR_RGBA(0, 0, 0, 220);
    u32 colorBot = GFX_COLOR_RGBA(50, 50, 80, 240);
    
    float uiOffset = offset * 0.5f; 
    
    GFX_DrawRectangle(-20 + uiOffset, currentBoxY, 0.8f, 450.0f, boxH, colorTop, colorTop, colorBot, colorBot);
    GFX_DrawRectSolid(-20 + uiOffset, currentBoxY, 0.85f, 450.0f, 2.0f, GFX_COLOR_RGBA(255, 204, 0, 255));

    float textX = 160.0f; 
    float textY = currentBoxY + 15.0f;
    
    if (currentBoxY < 240.0f) {
        GFX_DrawText(&currentTextObj, textX + uiOffset, textY, 0.9f, 0.4f, 0.4f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(255, 255, 255, 255));
    }

    if (charBaseX + charW > -200.0f) {
        float x = charBaseX; 
        switch(VOICEACT) {
            case 1:        
                switch(currentDisplayedCharId) {
                    case 0:
                        GFX_DrawImageAt(kun_excited, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break;
                    case 1:
                        GFX_DrawImageAt(kun_idle_speak, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break; 
                    case 2:
                        GFX_DrawImageAt(kun_idle, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break;
                    case 3:
                        GFX_DrawImageAt(kun_diss_speak, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break; 
                    case 4:
                        GFX_DrawImageAt(kun_diss, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break;
                }
                break;
            case 2:
                
                switch(currentDisplayedCharId) {
                    case 0:
                        GFX_DrawImageAt(chan_excited, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break;
                    case 1:
                        GFX_DrawImageAt(chan_idle_speak, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break; 
                    case 2:
                        GFX_DrawImageAt(chan_idle, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break;
                    case 3:
                        GFX_DrawImageAt(chan_diss_speak, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break; 
                    case 4:
                        GFX_DrawImageAt(chan_diss, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break;
                }
                break;
            case 3:
                
                switch(currentDisplayedCharId) {
                    case 0:
                        GFX_DrawImageAt(arhn_siema, x, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break;
                    case 1:
                        GFX_DrawImageAt(arhn_explain, x, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break; 
                    case 2:
                        GFX_DrawImageAt(arhn_diss, x, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break;
                }
                break;
            default:
                switch(currentDisplayedCharId) {
                    case 0:
                        GFX_DrawImageAt(chan_excited, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break;
                    case 1:
                        GFX_DrawImageAt(chan_idle_speak, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break; 
                    case 2:
                        GFX_DrawImageAt(chan_idle, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break;
                    case 3:
                        GFX_DrawImageAt(chan_diss_speak, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break; 
                    case 4:
                        GFX_DrawImageAt(chan_diss, x - 15.0f, finalCharY - 30.0f, 1.0f, NULL, 1.0f, 1.0f);
                        break;
                }
        }
    }
    if (flashTimer < FLASH_DURATION) {
        float alpha = 1.0f - (flashTimer / FLASH_DURATION);
        u32 white = GFX_COLOR_RGBA(255, 255, 255, (u8)(255 * alpha));
        GFX_DrawRectSolid(0, 0, 1.0f, 400, 240, white);
    }
}

void sceneTutorialRender(void) {
    GFX_BeginSceneTop(0, true);
     
    drawTutorialTop(0.0f);

    if (slider > 0.0f) {
        GFX_BeginSceneTop(1, true);
        
        drawTutorialTop(slider * 5.0f);
    }

    GFX_BeginSceneBottom();
    

    GFX_DrawImageAt(bg_bottom, tut_bg_x, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
    if (VOICEACT != 3) {
        if (tutorialScript[currentStep].bottomImage != NULL) {
            GFX_IMAGE* img = tutorialScript[currentStep].bottomImage;

            float drawW = img->width * pop_ease.value;
            float drawH = img->height * pop_ease.value;

            float x = 160.0f - (drawW / 2.0f);
            float y = 120.0f - (drawH / 2.0f);

            GFX_DrawImageAt(img, x, y, 0.5f, NULL, pop_ease.value, pop_ease.value);
        }
    } else {
        if (tutorialScript_arhn[currentStep].bottomImage != NULL) {
            GFX_IMAGE* img = tutorialScript_arhn[currentStep].bottomImage;

            float drawW = img->width * pop_ease.value;
            float drawH = img->height * pop_ease.value;

            float x = 160.0f - (drawW / 2.0f);
            float y = 120.0f - (drawH / 2.0f);

            GFX_DrawImageAt(img, x, y, 0.5f, NULL, pop_ease.value, pop_ease.value);
        }
    }

    float w, h;
    GFX_TextGetDimensions(&promptText, 0.6f, 0.6f, &w, &h);
    
    u8 pulse = (u8)(155 + 100 * fabs(sinf(osGetTime() / 500.0f)));
    GFX_DrawText(&promptText, 160.0f - (w/2), 200.0f, 0.5f, 0.6f, 0.6f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, pulse));
    
    if (flashTimer < FLASH_DURATION) {
        float alpha = 1.0f - (flashTimer / FLASH_DURATION);
        u32 white = GFX_COLOR_RGBA(255, 255, 255, (u8)(255 * alpha));
        GFX_DrawRectSolid(0, 0, 1.0f, 320, 240, white);
    }
}

void sceneTutorialExit(void) {
    GFX_TextBufDelete(tutorialTextBuf);
    GFX_TextBufDelete(promptBuf);
    
    stopCwav(5 + currentStep + (26 * offset));

}