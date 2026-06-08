#include <3ds.h>
#include <citro2d.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include "drawing.h"
#include "cJSON.h"

#include "scene_init.h"
#include "scene_manager.h"
#include "main.h"
#include "sprites.h"
#include "inpost_api.h" 
#include "utils.h"

int VOICEACT = 0;
bool VA_YES = false;


#define MAKE_IPV4(a,b,c,d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))
#define NTP_IP MAKE_IPV4(51, 137, 137, 111) 

typedef struct NtpPacket {
    u8 li_vn_mode; u8 stratum; u8 poll; u8 precision;
    u32 rootDelay; u32 rootDispersion; u32 refId;
    u32 refTm_s; u32 refTm_f;
    u32 origTm_s; u32 origTm_f;
    u32 rxTm_s; u32 rxTm_f;
    u32 txTm_s; u32 txTm_f;
} NtpPacket;

typedef enum {
    STATE_CHECK_DSP,
    STATE_NO_DSP_FIRM,
    STATE_CHECK_WIFI,
    STATE_NO_WIFI,
    STATE_TEST_CONNECTION, 
    STATE_ATTEMPT_NTP,     
    STATE_WAIT_NTP,        
    STATE_CERT_ERROR,      
    STATE_ANIM_IN,
    STATE_WAIT_INPUT,
    STATE_ANIM_OUT,
    STATE_DONE
} InitState;

static InitState currentState;
static GFX_TEXTBUF staticTextBuf;
static GFX_TEXT txtNoWifi, txtCertError, txtNoDspFirm, txtNtpFixing, txtTitle, txtOptA, txtOptX, txtOptY, txtOptB;

static float animTimer = 0.0f;
static float animFactor = 0.0f; 
static const float ANIM_SPEED = 0.05f;
static bool testRequestSent = false;
static bool ntpAttempted = false; 


static int ntpSock = -1;
static u64 ntpStartTick = 0;
static u64 ntpTimeoutStart = 0;

static float easeOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float f = t - 1.0f;
    return 1.0f + c3 * (f * f * f) + c1 * (f * f);
}

static float easeInBack(float x) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1;
    return c3 * x * x * x - c1 * x * x;
}

static void onSelectionFinished(void) { }


#define NTP_TO_Y2K_SECONDS 3155673600ULL

// implemntation based on the one in luma3ds
static Result setSystemTime(u64 msSince1900) {
    Result res = 0;
    u64 msFrom1900to2000 = NTP_TO_Y2K_SECONDS * 1000ULL;

    if (msSince1900 < msFrom1900to2000) return -1; 
    s64 msY2k = (s64)(msSince1900 - msFrom1900to2000);

    
    if (R_FAILED(res = ptmSysmInit())) return res;
    if (R_FAILED(res = ptmSetsInit())) { ptmSysmExit(); return res; }
    if (R_FAILED(res = cfguInit())) { ptmSetsExit(); ptmSysmExit(); return res; }

    
    res = PTMSYSM_SetRtcTime(msY2k);
    
    
    if (R_SUCCEEDED(res)) {
        res = PTMSYSM_SetUserTime(msY2k);
    }

    
    if (R_SUCCEEDED(res)) {
        res = PTMSETS_SetSystemTime(msY2k);
    }

    cfguExit();
    ptmSetsExit();
    ptmSysmExit();
    return res;
}

static void drawRoundedBoxGradient(float x, float y, float z, float w, float h, u32 cTop, u32 cBot, float r) {
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    GFX_DrawCircleSolid(x + r, y + r, z, r, cTop);                 
    GFX_DrawCircleSolid(x + w - r, y + r, z, r, cTop);             
    GFX_DrawCircleSolid(x + r, y + h - r, z, r, cBot);             
    GFX_DrawCircleSolid(x + w - r, y + h - r, z, r, cBot);         
    GFX_DrawRectangle(x, y + r, z, r, h - 2 * r, cTop, cTop, cBot, cBot);
    GFX_DrawRectangle(x + w - r, y + r, z, r, h - 2 * r, cTop, cTop, cBot, cBot);
    GFX_DrawRectangle(x + r, y, z, w - 2 * r, h, cTop, cTop, cBot, cBot);
}

static void drawRoundedBoxSolid(float x, float y, float z, float w, float h, u32 color, float r) {
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    GFX_DrawRectSolid(x + r, y, z, w - 2 * r, h, color);
    GFX_DrawRectSolid(x, y + r, z, r, h - 2 * r, color);
    GFX_DrawRectSolid(x + w - r, y + r, z, r, h - 2 * r, color);
    GFX_DrawCircleSolid(x + r, y + r, z, r, color);
    GFX_DrawCircleSolid(x + w - r, y + r, z, r, color);
    GFX_DrawCircleSolid(x + r, y + h - r, z, r, color);
    GFX_DrawCircleSolid(x + w - r, y + h - r, z, r, color);
}

int random_val; 

void sceneInitInit(void) {
    VA_YES = true;
    currentState = STATE_CHECK_DSP;
    animTimer = 0.0f;
    animFactor = 0.0f;
    testRequestSent = false;
    ntpAttempted = false; 
    srand(time(NULL));

    random_val = rand() % 500;  
    staticTextBuf = GFX_TextBufNew(4096);

    GFX_TextParse(&txtNoWifi, staticTextBuf, "Brak Internetu, podłącz aplikacje do internetu");
    GFX_TextParse(&txtCertError, staticTextBuf, "CERT ERROR!\nCzas w 3DS'ie jest niepoprawny, zmień go na aktualny\nWciśnij (START) by wyjść.");
    GFX_TextParse(&txtNtpFixing, staticTextBuf, "Błąd SSL. Próba naprawy czasu (NTP)...");
    GFX_TextParse(&txtNoDspFirm, staticTextBuf, "Brak oprogramowania DSP.\nW Rosalina Menu (L + D-Pad dół + Select)\nwybierz \"Miscellaneous options\",\na następnie \"Dump DSP firmware\"\nWciśnij (START) by wyjść.");
    
    GFX_TextParse(&txtTitle, staticTextBuf, "Wybierz głos nawigatora głosowego");
    
    GFX_TextParse(&txtOptA, staticTextBuf, "(A) Mężczyzna");
    GFX_TextParse(&txtOptX, staticTextBuf, "(X) Kobieta");
    GFX_TextParse(&txtOptY, staticTextBuf, "(Y) Niebinarny");
    GFX_TextParse(&txtOptB, staticTextBuf, "(B) Brak");

    GFX_TextOptimize(&txtNoWifi);
    GFX_TextOptimize(&txtCertError);
    GFX_TextOptimize(&txtNtpFixing);
    GFX_TextOptimize(&txtTitle);
    GFX_TextOptimize(&txtOptA);
    GFX_TextOptimize(&txtOptX);
    GFX_TextOptimize(&txtOptY);
    GFX_TextOptimize(&txtOptB);
    
    if (access("/3ds/InPost3DS/opcje.json", F_OK) == 0) {
        cJSON *jsonfl = NULL;
        open_json("/3ds/InPost3DS/opcje.json", &jsonfl);
        cJSON *czyjest = cJSON_GetObjectItem(jsonfl, "VA");

        VOICEACT = (int)cJSON_GetNumberValue(czyjest);
        
        if (VOICEACT < 0) VOICEACT = 0;
        if (VOICEACT > 3) VOICEACT = 3;

        cJSON_Delete(jsonfl);
    } else {
        VA_YES = false;
    }
}

void sceneInitUpdate(uint32_t kDown, uint32_t kHeld) {
    if (random_val != 213) {
        switch (currentState) {
            case STATE_CHECK_DSP:
                if (access("/3ds/dspfirm.cdc", F_OK) == 0) {
                    currentState = STATE_CHECK_WIFI;
                } else {
                    currentState = STATE_NO_DSP_FIRM;
                }
                break;
            case STATE_NO_DSP_FIRM:
                if (kDown & KEY_START) {

                }
                break;
            case STATE_CHECK_WIFI:
                if (osGetWifiStrength() > 0) {
                    currentState = STATE_TEST_CONNECTION;
                    testRequestSent = false;
                    testreq.done = false;
                    testreq.size = 0;
                    ntpAttempted = false; 
                } else {
                    currentState = STATE_NO_WIFI;
                }
                break;

            case STATE_NO_WIFI:
                if (osGetWifiStrength() > 0) {
                    currentState = STATE_TEST_CONNECTION;
                    testRequestSent = false;
                    testreq.done = false; 
                    testreq.size = 0;
                    
                    ntpAttempted = false; 
                }
                break;

            case STATE_TEST_CONNECTION:
                if (!testRequestSent) {
                    testrequest();
                    testRequestSent = true;
                }

                if (testreq.done) {
                    if (testreq.size > 5) {
                        
                        if (!VA_YES){
                            currentState = STATE_ANIM_IN;
                        } else {
                            currentState = STATE_DONE;
                            onSelectionFinished();
                        }
                        animTimer = 0.0f;
                    } else {
                        
                        if (!ntpAttempted) {
                            currentState = STATE_ATTEMPT_NTP;
                        } else {
                            currentState = STATE_CERT_ERROR;
                        }
                    }
                }
                break;

            
            case STATE_ATTEMPT_NTP:
                ntpAttempted = true; 
                
                
                ntpSock = socket(AF_INET, SOCK_DGRAM, 0);
                
                
                if (ntpSock < 0) {
                    
                    
                    currentState = STATE_CHECK_WIFI; 
                    ntpAttempted = false; 
                    break;
                }
                

                struct sockaddr_in servAddr = {0};
                servAddr.sin_family = AF_INET;
                servAddr.sin_addr.s_addr = htonl(NTP_IP);
                servAddr.sin_port = htons(123);

                int flags = fcntl(ntpSock, F_GETFL, 0);
                fcntl(ntpSock, F_SETFL, flags | O_NONBLOCK);

                NtpPacket packet = {0};
                packet.li_vn_mode = 0x1b; 

                sendto(ntpSock, &packet, sizeof(NtpPacket), 0, (struct sockaddr *)&servAddr, sizeof(servAddr));
                
                ntpStartTick = svcGetSystemTick();
                ntpTimeoutStart = osGetTime();
                currentState = STATE_WAIT_NTP;
                break;

            case STATE_WAIT_NTP:
                {
                    NtpPacket packet = {0};
                    struct sockaddr_in fromAddr;
                    socklen_t fromLen = sizeof(fromAddr);

                    int ret = recvfrom(ntpSock, &packet, sizeof(NtpPacket), 0, (struct sockaddr *)&fromAddr, &fromLen);
                    
                    if (ret > 0) {
                        u64 endTick = svcGetSystemTick();
                        close(ntpSock); 
                        ntpSock = -1;

                        u32 txSeconds = ntohl(packet.txTm_s);
                        u32 txFraction = ntohl(packet.txTm_f);

                        u64 roundTripTicks = endTick - ntpStartTick;
                        u64 dt = (u64)((1000.0 * roundTripTicks) / (SYSCLOCK_ARM11)); 

                        u64 msFraction = ((u64)txFraction * 1000ULL) >> 32;
                        u64 msSince1900 = ((u64)txSeconds * 1000ULL) + msFraction + dt;

                        Result timeRes = setSystemTime(msSince1900);
                        if (R_SUCCEEDED(timeRes)) {
                            currentState = STATE_TEST_CONNECTION; 
                        } else {
                            currentState = STATE_CERT_ERROR;
                        }

                        testRequestSent = false;
                        testreq.done = false;
                    }
                    else {
                        if (osGetTime() - ntpTimeoutStart > 5000) {
                            close(ntpSock);
                            ntpSock = -1;
                            currentState = STATE_CERT_ERROR; 
                        }
                    }
                }
                break;
            

            case STATE_CERT_ERROR:
                if (kDown & KEY_START) {
                    
                }
                break;

            case STATE_ANIM_IN:
                animTimer += ANIM_SPEED;
                if (animTimer >= 1.0f) {
                    animTimer = 1.0f;
                    currentState = STATE_WAIT_INPUT;
                }
                animFactor = easeOutBack(animTimer);
                break;

            case STATE_WAIT_INPUT:
                animFactor = 1.0f; 
                if (kDown & (KEY_A | KEY_X | KEY_Y | KEY_B)) {
                    if (kDown & KEY_A) VOICEACT = 1;
                    else if (kDown & KEY_X) VOICEACT = 2;
                    else if (kDown & KEY_Y) VOICEACT = 3;
                    else if (kDown & KEY_B) VOICEACT = 0;
                    
                    cJSON *oproot = cJSON_CreateObject();
                    cJSON_AddItemToObject(oproot, "VA", cJSON_CreateNumber(VOICEACT));
                    save_json("/3ds/InPost3DS/opcje.json", oproot);
                    cJSON_Delete(oproot);
                    
                    currentState = STATE_ANIM_OUT;
                    animTimer = 0.0f;
                }
                break;

            case STATE_ANIM_OUT:
                animTimer += ANIM_SPEED;
                if (animTimer >= 1.0f) {
                    animTimer = 1.0f;
                    currentState = STATE_DONE;
                    onSelectionFinished();
                }
                float exitScale = easeInBack(animTimer);
                animFactor = 1.0f - exitScale;
                if (animFactor < 0.0f) animFactor = 0.0f;
                break;

            case STATE_DONE:
                sceneManagerSwitchTo(SCENE_INTRO);
                break;
        }
    } 
}

void sceneInitRender(void) {
    float screenW = 400.0f;
    float screenH = 240.0f;
    GFX_BeginSceneTop(0, true);
    
    
    if (currentState == STATE_NO_WIFI) {
        float textW = 0, textH = 0;
        GFX_TextGetDimensions(&txtNoWifi, 0.5f, 0.5f, &textW, &textH);
        GFX_DrawText(&txtNoWifi, (screenW - textW)/2, (screenH - textH)/2, 0, 0.5f, 0.5f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(255, 0, 0, 255));
        return; 
    }

    if (currentState == STATE_CERT_ERROR) {
        float textW = 0, textH = 0;
        GFX_TextGetDimensions(&txtCertError, 0.5f, 0.5f, &textW, &textH);
        GFX_DrawText(&txtCertError, (screenW - textW)/2, (screenH - textH)/2, 0, 0.5f, 0.5f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(255, 0, 0, 255));
        return;
    }

    if (currentState == STATE_NO_DSP_FIRM) {
        float textW = 0, textH = 0;
        GFX_TextGetDimensions(&txtNoDspFirm, 0.5f, 0.5f, &textW, &textH);
        GFX_DrawText(&txtNoDspFirm, (screenW - textW)/2, (screenH - textH)/2, 0, 0.5f, 0.5f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(255, 0, 0, 255));
        return;
    }

    
    if (currentState == STATE_ATTEMPT_NTP || currentState == STATE_WAIT_NTP) {
        float textW = 0, textH = 0;
        GFX_TextGetDimensions(&txtNtpFixing, 0.5f, 0.5f, &textW, &textH);
        GFX_DrawText(&txtNtpFixing, (screenW - textW)/2, (screenH - textH)/2, 0, 0.5f, 0.5f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(255, 255, 0, 255));
        return;
    }

    if (currentState >= STATE_ANIM_IN && currentState <= STATE_ANIM_OUT) {
        float alphaFactor = (currentState == STATE_ANIM_OUT) ? animFactor : (animTimer > 1.0f ? 1.0f : animTimer); 
        u8 dimAlpha = (u8)(150.0f * alphaFactor);
        GFX_DrawRectSolid(0, 0, 0.5f, screenW, screenH, GFX_COLOR_RGBA(0, 0, 0, dimAlpha));
        
        float finalBoxW = 320.0f;
        float finalBoxH = 180.0f;

        float curW = finalBoxW * animFactor;
        float curH = finalBoxH * animFactor;
        
        float curX = (screenW - curW) / 2.0f;
        float curY = (screenH - curH) / 2.0f;
        
        u32 clrBorder = GFX_COLOR_RGBA(255, 255, 255, 255); 
        u32 clrBgTop = GFX_COLOR_RGBA(45, 45, 20, 255); 
        u32 clrBgBot = GFX_COLOR_RGBA(5, 5, 5, 255);    
        
        float cornerRadius = 5.0f * animFactor; 
        float borderThickness = 3.0f;

        if (curW > cornerRadius * 2) {
            drawRoundedBoxSolid(curX, curY, 0.6f, curW, curH, clrBorder, cornerRadius);
            drawRoundedBoxGradient(
                curX + borderThickness, 
                curY + borderThickness, 
                0.6f, 
                curW - (borderThickness * 2), 
                curH - (borderThickness * 2), 
                clrBgTop,
                clrBgBot, 
                cornerRadius - 1.0f
            );
        }

        float textScaleBase = 0.5f; 
        float curTextScale = textScaleBase * animFactor;
        
        if (curTextScale > 0.01f) {
            float wT, hT;
            GFX_TextGetDimensions(&txtTitle, curTextScale, curTextScale, &wT, &hT);
            u32 clrTitle = GFX_COLOR_RGBA(255, 235, 59, 255);

            GFX_DrawText(&txtTitle, 
                (screenW - wT)/2.0f, 
                curY + (15.0f * animFactor), 
                0.7f, curTextScale, curTextScale, GFX_ALIGN_LEFT, clrTitle);

            float optScale = 0.6f * animFactor;
            float startY = curY + (60.0f * animFactor);
            float gapY = 28.0f * animFactor;
            u32 clrText = GFX_COLOR_RGBA(255, 255, 255, 255);

            float wX, hX;
            GFX_TextGetDimensions(&txtOptX, optScale, optScale, &wX, &hX);
            GFX_DrawText(&txtOptX, (screenW - wX)/2, startY, 0.7f, optScale, optScale, GFX_ALIGN_LEFT, clrText);

            float wA, hA;
            GFX_TextGetDimensions(&txtOptA, optScale, optScale, &wA, &hA);
            GFX_DrawText(&txtOptA, (screenW - wA)/2, startY + gapY, 0.7f, optScale, optScale, GFX_ALIGN_LEFT, clrText);

            float wY_dim, hY_dim;
            GFX_TextGetDimensions(&txtOptY, optScale, optScale, &wY_dim, &hY_dim);
            GFX_DrawText(&txtOptY, (screenW - wY_dim)/2, startY + gapY*2, 0.7f, optScale, optScale, GFX_ALIGN_LEFT, clrText);

            float wB, hB;
            GFX_TextGetDimensions(&txtOptB, optScale, optScale, &wB, &hB);
            GFX_DrawText(&txtOptB, (screenW - wB)/2, startY + gapY*3, 0.7f, optScale, optScale, GFX_ALIGN_LEFT, clrText);
        }
    }
    GFX_BeginSceneBottom();
    
    if (random_val == 213) {
        GFX_DrawImageAt(sie_do_wiezienia, 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);
    }
}

void sceneInitExit(void) {
    if (ntpSock >= 0) {
        close(ntpSock);
    }
    GFX_TextBufDelete(staticTextBuf);
}