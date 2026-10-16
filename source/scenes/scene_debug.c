#include <3ds.h>
#include <citro2d.h>
#include "scene_debug.h"
#include "scene_manager.h"
#include "../main.h"
#include "../utils/sprites.h"
#include "../inpost/inpost_api.h"
#include "../utils/text.h"
#include "../HAL/drawing.h"

#define DEBUG_MENU_ITEMS 6

bool debugmode = false;
int debugMenuSelected = 0;

const float menuStartY = 100.0f;
const float menuLineHeight = 28.0f;

bool paczkashow = false;
bool fake_paczka_mode = false;

GFX_TEXTBUF DebugMenu;
GFX_TEXT debugmenu_Text[50];


void RebuildDebugMenuStaticText(void) {
    GFX_TextParse(&debugmenu_Text[0], DebugMenu, "INPOST3DS debug menu");
    GFX_TextParse(&debugmenu_Text[1], DebugMenu, "Zbierz dane do testów (logi)");
    GFX_TextParse(&debugmenu_Text[2], DebugMenu, "Odśwież token (/v1/authenticate)");
    GFX_TextParse(&debugmenu_Text[3], DebugMenu, "Pokaż Fake'owe paczki (romfs://test_data.json)");
    GFX_TextParse(&debugmenu_Text[4], DebugMenu, "Pokaż prawdziwe paczki (/parcels/tracked)");
    GFX_TextParse(&debugmenu_Text[5], DebugMenu, "Tutorial Menu");
    GFX_TextParse(&debugmenu_Text[10], DebugMenu, ">");
    
    GFX_TextOptimize(&debugmenu_Text[10]);
    for (int i = 0; i < 6; i++) 
        GFX_TextOptimize(&debugmenu_Text[i]);
}

void sceneDebugInit(void) {
    DebugMenu = GFX_TextBufNew(4096); 
    GFX_TextBufClear(DebugMenu);
    RebuildDebugMenuStaticText();
}

void sceneDebugUpdate(uint32_t kDown, uint32_t kHeld) {
    if (kDown & KEY_DOWN) {
        debugMenuSelected++;
        if (debugMenuSelected >= DEBUG_MENU_ITEMS)
            debugMenuSelected = 0;
    }

    if (kDown & KEY_UP) {
        debugMenuSelected--;
        if (debugMenuSelected < 0)
            debugMenuSelected = DEBUG_MENU_ITEMS - 1;
    }

    if (kDown & KEY_A) {
        switch (debugMenuSelected) {
            case 0:
                getEverything();
                break;

            case 1:
                refresh_the_Token(refreshToken);
                break;

            case 2: 
                paczkashow = false;
                
                
                GFX_TextBufClear(DebugMenu);
                RebuildDebugMenuStaticText(); 
                
                
                parseFakePaczkas(false);
                
                if (paczka_count > 0) {
                    GFX_TextParse(&debugmenu_Text[11], DebugMenu, paczka_list[0].pickupPointName);
                    GFX_TextParse(&debugmenu_Text[12], DebugMenu, paczka_list[0].city);
                    GFX_TextParse(&debugmenu_Text[13], DebugMenu, paczka_list[0].status);
                    GFX_TextParse(&debugmenu_Text[14], DebugMenu, paczka_list[0].shipmentNumber);
                }
                if (paczka_count > 1) {
                    GFX_TextParse(&debugmenu_Text[15], DebugMenu, paczka_list[1].pickupPointName);
                    GFX_TextParse(&debugmenu_Text[16], DebugMenu, paczka_list[1].city);
                    GFX_TextParse(&debugmenu_Text[17], DebugMenu, paczka_list[1].status);
                    GFX_TextParse(&debugmenu_Text[18], DebugMenu, paczka_list[1].shipmentNumber);
                }
                paczkashow = true;
                break;

            case 3: 
                paczkashow = false;

                
                GFX_TextBufClear(DebugMenu);
                RebuildDebugMenuStaticText();
                

                parsePaczkas((const char*)paczkas.data, false);

                if (paczka_count > 0) {
                    GFX_TextParse(&debugmenu_Text[11], DebugMenu, paczka_list[0].pickupPointName);
                    GFX_TextParse(&debugmenu_Text[12], DebugMenu, paczka_list[0].city);
                    GFX_TextParse(&debugmenu_Text[13], DebugMenu, paczka_list[0].status);
                    GFX_TextParse(&debugmenu_Text[14], DebugMenu, paczka_list[0].shipmentNumber);
                }
                if (paczka_count > 1) {
                    GFX_TextParse(&debugmenu_Text[15], DebugMenu, paczka_list[1].pickupPointName);
                    GFX_TextParse(&debugmenu_Text[16], DebugMenu, paczka_list[1].city);
                    GFX_TextParse(&debugmenu_Text[17], DebugMenu, paczka_list[1].status);
                    GFX_TextParse(&debugmenu_Text[18], DebugMenu, paczka_list[1].shipmentNumber);
                }
                paczkashow = true;
                break;

            case 4:
                sceneManagerSwitchTo(SCENE_TUTORIAL);
                break;
        }
    }
}

void sceneDebugRender(void) {
    GFX_BeginSceneTop(0, true);
    

    
    GFX_DrawShadowedText(
        &debugmenu_Text[0],
        200.0f, 20.0f, 1.0f, 1.0f, 1.0f, GFX_ALIGN_CENTER,
        GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff),
        GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff)
    );

    
    for (int i = 0; i < DEBUG_MENU_ITEMS; i++) {
        float y = menuStartY + (i * menuLineHeight);

        
        if (i == debugMenuSelected) {
            GFX_DrawText(
                &debugmenu_Text[10],
                15.0f, y,
                0.5f, 0.5f, 0.5f, GFX_ALIGN_LEFT,
                GFX_COLOR_RGBA(255, 255, 255, 255)
            );
        }

        u32 color = (i == debugMenuSelected)
            ? GFX_COLOR_RGBA(255, 255, 0, 255)
            : GFX_COLOR_RGBA(200, 200, 200, 255);

        GFX_DrawText(
            &debugmenu_Text[i + 1],
            30.0f, y,
            0.5f, 0.5f, 0.5f, GFX_ALIGN_LEFT,
            color
        );
    }

    GFX_BeginSceneBottom();
    
    
    if (paczkashow) {
        
        if (paczka_count > 0) {
            float y = 15.0f;
            for(int k=11; k<=14; k++) {
                GFX_DrawText(&debugmenu_Text[k], 15.0f, y, 0.5f, 0.5f, 0.5f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(255, 255, 255, 255));
                y += 15.0f;
            }
        }
        if (paczka_count > 1) {
            float y = 90.0f;
            for(int k=15; k<=18; k++) {
                GFX_DrawText(&debugmenu_Text[k], 15.0f, y, 0.5f, 0.5f, 0.5f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(255, 255, 255, 255));
                y += 15.0f;
            }
        }
    }
}

void sceneDebugExit(void) {
    GFX_TextBufDelete(DebugMenu);
}