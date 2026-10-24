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
#include "utils/logs.h"
#include "utils/cwav_shit.h"
#include "scenes/scene_manager.h"
#include "scenes/scene_debug.h"
#include "main.h"
#include "utils/sprites.h"
#include "utils/request.h"
#include "HAL/drawing.h"
#include "HAL/data.h"

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x300000


struct memory {
    char *response;
    size_t size;
};

extern int cwavCount;
size_t linear_bytes_used = 0;

static u32 *SOC_buffer = NULL;

u8* captureBuf = NULL;
ndspWaveBuf captureWaveBuf; 
const u32 CAPTURE_SIZE = 0x500; 

float slider;
bool saving_va = false;


int main(int argc, char* argv[]) {
    cwavUseEnvironment(CWAV_ENV_DSP);
    romfsInit();
    cfguInit();
    GFX_InitGfx();
    ndspInit();
    init_logger();
    doing_debug_logs = false;
    INPUT_Setup();
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (!SOC_buffer || socInit(SOC_buffer, SOC_BUFFERSIZE)) {
        printf("SOC init failed\n");
    }

    consoleClear();
    start_request_thread();
 
    osSetSpeedupEnable(true);
    

    
    initCwavSystem();
    captureBuf = linearAlloc(CAPTURE_SIZE);
    
    memset(&captureWaveBuf, 0, sizeof(ndspWaveBuf)); 
    captureWaveBuf.data_vaddr = captureBuf;         
    captureWaveBuf.nsamples = CAPTURE_SIZE / 4;      
    captureWaveBuf.looping = true;                   
    
    ndspSetCapture(&captureWaveBuf);  
     
    spritesInit();

    sceneManagerSwitchTo(SCENE_INIT);


    
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