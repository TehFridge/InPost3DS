#include <citro2d.h>
#include <3ds.h>
#include <3ds/synchronization.h>
#include <cwav.h>
#include <ncsnd.h>
#include <jansson.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include "logs.h"
#include "cwav_shit.h"
#include "scene_manager.h"
#include "scene_debug.h"
#include "main.h"
#include "sprites.h"
#include "request.h"
#include "drawing.h"
#include "data.h"

#define FRAME_MS (1000.0f / 60.0f)
#define MAX_SPRITES   1
#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x300000


#define fmin(a, b) ((a) < (b) ? (a) : (b))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

struct memory {
    char *response;
    size_t size;
};

bool cpu_debug = false;
extern bool logplz, readingoferta, json_done, citra_machen;
extern float text_w, text_h, max_scroll;

extern int cwavCount;
size_t linear_bytes_used = 0;
bool pausedForSleep = false;
int Scene;
int selectioncodelol;
bool przycskmachen = true;
bool generatingQR = false;
char combinedText[128];
static u32 *SOC_buffer = NULL;

GFX_TEXTBUF totpBuf;
GFX_TEXTBUF g_staticBuf, kupon_text_Buf, themeBuf;
GFX_TEXT g_staticText[100];
GFX_TEXT themeText[100];
GFX_TEXT g_totpText[5];



u8* captureBuf = NULL;
ndspWaveBuf captureWaveBuf; 
const u32 CAPTURE_SIZE = 0x340; 

float slider;
bool saving_va = false;
bool debug_stats = false;


int main(int argc, char* argv[]) {
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (!SOC_buffer || socInit(SOC_buffer, SOC_BUFFERSIZE)) {
        printf("SOC init failed\n");
    }

    if (link3dsConnectToHost(true, true) >= 0) {
        printf("Connected to 3dslink\n");
    }

    cwavUseEnvironment(CWAV_ENV_DSP);
    romfsInit();
    cfguInit();
    GFX_InitGfx();
    ndspInit();
    init_logger();
    doing_debug_logs = false;
    INPUT_Setup();

    totpBuf = GFX_TextBufNew(128);
    g_staticBuf = GFX_TextBufNew(256);
    kupon_text_Buf = GFX_TextBufNew(512);
    themeBuf = GFX_TextBufNew(512);
    
    
    GFX_TEXTBUF memBuf = GFX_TextBufNew(1024); 
    GFX_TEXT memtext[6]; 
    memset(memtext, 0, sizeof(memtext));

    consoleClear();
    start_request_thread();
 
    osSetSpeedupEnable(true);
    

    
    initCwavSystem();
    Scene = 0;
    captureBuf = linearAlloc(CAPTURE_SIZE);
    
    memset(&captureWaveBuf, 0, sizeof(ndspWaveBuf)); 
    captureWaveBuf.data_vaddr = captureBuf;         
    captureWaveBuf.nsamples = CAPTURE_SIZE / 4;      
    captureWaveBuf.looping = true;                   
    
    ndspSetCapture(&captureWaveBuf);  
     
    spritesInit();

    sceneManagerSwitchTo(SCENE_INIT);


    
    GFX_TextBufClear(memBuf);
    FS_CheckIfInPostIstnieje();

    while (aptMainLoop()) {
        INPUT_Scan();
        slider = osGet3DSliderState();

        if ((kDown & KEY_START) || saving_va)
            break;
        #ifdef DEBUG
            if ((kDown & KEY_Y) && debugmode){
                debug_stats = !debug_stats;
                logplz = !logplz;
            }
        #endif

        sceneManagerUpdate(kDown, kHeld);
        GFX_RenderFrame();
    }

    close_logger();

    GFX_TextBufDelete(g_staticBuf);
    GFX_TextBufDelete(kupon_text_Buf);
    GFX_TextBufDelete(themeBuf);
    GFX_TextBufDelete(totpBuf);
    GFX_TextBufDelete(memBuf);
    socExit();
    if (SOC_buffer) free(SOC_buffer); 

    C2D_Fini();
    C3D_Fini();
    ndspExit();
    cfguExit();
    romfsExit();
    gfxExit();

    return 0;
}