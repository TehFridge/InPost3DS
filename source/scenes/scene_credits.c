#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <math.h> 
#include "scene_credits.h"
#include "scene_manager.h"
#include "../main.h"
#include "../utils/sprites.h"
#include "../utils/cwav_shit.h"
#include "../utils/text.h"
#include "scene_mainmenu.h"
#include "../HAL/drawing.h"



#define STATE_FLASH      0
#define STATE_LOGO_FADE  1
#define STATE_WAIT       2
#define STATE_SCROLL     3


static int sceneState = 0;
static float flashAlpha = 255.0f; 
static float logoAlpha = 0.0f;    
static float waveAlpha = 0.0f;    
static float scrollY = 480.0f;    
static u64 stateTimer = 0;        
static u64 sceneStartTime = 0;    
static bool musicStarted = false; 


static GFX_TEXTBUF creditBuf;
static GFX_TEXT creditText[1000];   
static GFX_TEXT exitText;           

static const char* creditLines[] = {
    "InPost3DS",
    "(Codename: ShroomPost)",
    "",
    "",
    "PROGRAMOWANIE",
    "TehFridge",
    "",
    "",
    "",
    "GRAFIKA",
    "",
    "UI Design",
    "TehFridge",
    "",
    "InPost Chan/Kun Design",
    "Sansy (@fedora_maniac)",
    "",
    "Archon Mii Design",
    "Arkadiusz \"Dark Archon\" Kamiński",
    "",
    "Easter Egg",
    "Chiyoko",
    "",
    "",
    "",
    "UDŹWIĘKOWIENIE",
    "",
    "InPost Chan VA",
    "Sansy (@fedora_maniac)",
    "",
    "InPost Kun VA",
    "Nishi Yuuma",
    "",
    "Archon VA",
    "Arkadiusz \"Dark Archon\" Kamiński",
    "",
    "Banner Jingle",
    "Chiyoko",
    "",
    "Main Menu Music",
    "John \"Joy\" Tay",
    "",
    "Credits Music",
    "Takenobu Mitsuyoshi",
    "Naofumi Hataya",
    "Tomoko Sasaki",
    "",
    "Nadaj Menu Music",
    "Rei8Bit",
    "",
    "INPOST3DS BETATESTING TEAM",
    "",
    "MrowA Matsuri",
    "Vahn Blackwood",
    "igordes",
    "Chiyoko",
    "Szprink",
    "Wafelq",
    "mmaacio",
    "fifi_sc",
    "Reisu",
    "Plesio",
    "niishee",
    "",
    "Additional pozdrowienia dla:",
    "akirsonki",
    "Jesteś cool girl",
    "",
    "gencii",
    "Dzięki za ładowarke",
    "oraz fanarcika <3",
    "",
    "BartkaGM",
    "Kiedy nowe NGW :tf:", 
    "",
    "oraz wszystkich Remconowiczów",
    "którzy byli na prelekcji", 
    "Dzięki za przyjście :D", 
    "(AMBEREXPO odśnieżcie dach plz)", 
    "",
    "Serdeczne podziękowania dla",
    "wszystkich którzy wspierali",
    "mnie podczas tworzenia tego",
    "projektu i w życiu prywatnym.",
    ":D",
    "",
    "Projekt korzysta z API Aplikacji",
    "InPost Mobile",
    "",
    "InPost nie jest w żaden sposób",
    "powiązany z tym projektem.",
    "",
    "Wszystkie znaki towarowe",
    "należą do ich właścicieli.",
    "",
    ""
};
#define CREDIT_LINE_COUNT (sizeof(creditLines)/sizeof(creditLines[0]))

void drawCaptureWaveform(float alphaVal) {
    if (!captureBuf || alphaVal <= 0.0f) return;

    GSPGPU_InvalidateDataCache(captureBuf, CAPTURE_SIZE);
    s16* pcmData = (s16*)captureBuf;

    musicisplaying = false;
    for (int i = 0; i < 100; i++) {
        s16 sample = pcmData[i];
        s16 absSample = (sample ^ (sample >> 15)) - (sample >> 15);
        if (absSample > 100) { 
            musicisplaying = true;
            break;
        }
    }

    int step = 1; 

    u8 lineAlpha = (u8)((178.0f * alphaVal) / 255.0f);
    u8 shadowAlpha = (u8)alphaVal;
    u32 lineColor = GFX_COLOR_RGBA(192, 255, 0, lineAlpha); 
    u32 shadowColor = GFX_COLOR_RGBA(0, 0, 0, shadowAlpha);

    float prevX = 0.0f;
    float prevY = 120.0f;
    float smoothedY = 120.0f; 

    const float normalizeMult = 80.0f / 32768.0f;

    // --- 2. FAST RENDER WITH SPATIAL GLUE ---
    for (int x = 0; x < 400; x += 2) {
        int readIndex = x * step;
        if (readIndex >= (CAPTURE_SIZE / 2)) break;

        s16 sample = pcmData[readIndex];
        
        float rawY = 120.0f + ((float)sample * normalizeMult);

        if (x == 0) {
            smoothedY = rawY;
        } else {
            smoothedY += (rawY - smoothedY) * 0.45f; 
        }

        float currentX = (float)x;
        float currentY = smoothedY;

        if (x > 0) {
            GFX_DrawLine(
                prevX, prevY + 3.0f, shadowColor, 
                currentX, currentY + 3.0f, shadowColor,
                2.0f, 0.5f 
            );
            GFX_DrawLine(
                prevX, prevY, lineColor,   
                currentX, currentY, lineColor,
                2.0f, 0.5f 
            );
        }
        
        prevX = currentX;
        prevY = currentY;
    }
}

void sceneCreditsInit(void) {
    
    sceneState = STATE_FLASH;
    flashAlpha = 255.0f;
    logoAlpha = 0.0f;
    waveAlpha = 0.0f; 
    scrollY = 480.0f; 
    
    stateTimer = 0;
    sceneStartTime = osGetTime(); 
    musicStarted = false;

    
    creditBuf = GFX_TextBufNew(4096);
    for (int i = 0; i < CREDIT_LINE_COUNT; i++) {
        GFX_TextParse(&creditText[i], creditBuf, creditLines[i]);
        GFX_TextOptimize(&creditText[i]);
    }

    
    GFX_TextParse(&exitText, creditBuf, "Wciśnij (B) by wyjść");
    GFX_TextOptimize(&exitText);
}

void sceneCreditsUpdate(uint32_t kDown, uint32_t kHeld) {
    
    if (kDown & (KEY_B | KEY_START)) {
        sceneManagerSwitchTo(SCENE_HOME_MENU); 
        return; 
    }

    
    
    if (!musicStarted) {
        if (osGetTime() - sceneStartTime >= 1500) {
            playCwav(83, true); 
            musicStarted = true;
        }
    }

    

    
    if (sceneState == STATE_FLASH) {
        if (flashAlpha > 0.0f) {
            flashAlpha -= 5.0f; 
            if (flashAlpha < 0.0f) flashAlpha = 0.0f;
        } else {
            sceneState = STATE_LOGO_FADE;
        }
    }

    
    if (sceneState == STATE_LOGO_FADE) {
        if (logoAlpha < 255.0f) {
            logoAlpha += 2.0f; 
            if (logoAlpha > 255.0f) logoAlpha = 255.0f;
        } else {
            sceneState = STATE_WAIT;
            stateTimer = osGetTime();
        }
    }

    
    if (sceneState == STATE_WAIT) {
        if (osGetTime() - stateTimer >= 1500) { 
            sceneState = STATE_SCROLL;
        }
    }

    
    if (sceneState == STATE_SCROLL) {
        scrollY -= 0.5f; 
        
        
        if (waveAlpha < 255.0f) {
            waveAlpha += 2.0f; 
            if (waveAlpha > 255.0f) waveAlpha = 255.0f;
        }

        
        if (logoAlpha > 50.0f) logoAlpha -= 0.5f; 
    }
}


void drawTopContent(float offset, float alphaFade) {
    
    if (logo3ds->width) {

        C2D_ImageTint tint;
        C2D_AlphaImageTint(&tint, alphaFade); 
        
        
        GFX_DrawImageAt(logo3ds, 0, 0, 0.5f, &tint, 1.0f, 1.0f);
    }
    drawCaptureWaveform(waveAlpha);
    
    if (sceneState == STATE_SCROLL) {
        float currentY = scrollY; 
        
        for (int i = 0; i < CREDIT_LINE_COUNT; i++) {
            float width, height;
            GFX_TextGetDimensions(&creditText[i], 0.7f, 0.7f, &width, &height);
            
            
            float x = (400 - width) / 2;
            
            
            if (currentY + height > 0 && currentY < 240) {
                GFX_DrawText(&creditText[i], x + offset, currentY, 0.6f, 0.7f, 0.7f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(255, 255, 255, 255));
            }
            currentY += height + 10.0f;
        }
    }
}


void drawBottomContent(void) {
    if (sceneState == STATE_SCROLL) {
        
        
        
        float currentY = scrollY - 240.0f; 
        
        for (int i = 0; i < CREDIT_LINE_COUNT; i++) {
            float width, height;
            GFX_TextGetDimensions(&creditText[i], 0.7f, 0.7f, &width, &height);
            
            
            float x = (320 - width) / 2;
            
            
            if (currentY + height > 0 && currentY < 240) {
                GFX_DrawText(&creditText[i], x, currentY, 0.6f, 0.7f, 0.7f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(255, 255, 255, 255));
            }
            currentY += height + 10.0f;
        }
    }

    
    
    if (sceneState != STATE_FLASH) {
        
        
        
        u8 pulseAlpha = (u8)(fabs(sin(osGetTime() / 300.0)) * 255.0f);

        
        if (pulseAlpha < 50) pulseAlpha = 50;

        u32 exitColor = GFX_COLOR_RGBA(255, 255, 255, pulseAlpha);
        u32 exitShadow = GFX_COLOR_RGBA(235, 163, 7, pulseAlpha);

        
        
        GFX_DrawShadowedText(&exitText, 160.0f, 220.0f, 0.9f, 0.5f, 0.5f, GFX_ALIGN_CENTER, exitColor, exitShadow);
    }
}

void sceneCreditsRender(void) {
    float logoAlphaVal = logoAlpha / 255.0f;

    
    GFX_BeginSceneTop(0, true); 
    
    drawTopContent(0.0f, logoAlphaVal);
    
    if (flashAlpha > 0.0f) {
        GFX_DrawRectSolid(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255, 255, 255, (u8)flashAlpha));
    }

    
    if (slider > 0.0f) {
        GFX_BeginSceneTop(1, true);
        
        drawTopContent(slider * -5.0f, logoAlphaVal); 
        
        if (flashAlpha > 0.0f) {
            GFX_DrawRectSolid(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255, 255, 255, (u8)flashAlpha));
        }
    }

    
    GFX_BeginSceneBottom(); 
     
    drawBottomContent();
    
    if (flashAlpha > 0.0f) {
        GFX_DrawRectSolid(0, 0, 0, 320, 240, GFX_COLOR_RGBA(255, 255, 255, (u8)flashAlpha));
    }
}

void sceneCreditsExit(void) {
    if (creditBuf) GFX_TextBufDelete(creditBuf);
    musicisplaying = false;
    stopCwav(83);
}