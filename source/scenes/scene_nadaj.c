#include <3ds.h>
#include <citro2d.h>
#include "scene_nadaj.h"
#include "scene_manager.h"
#include "../main.h"
#include "../inpost/inpost_api.h"
#include "../utils/text.h"
#include "../utils/cwav_shit.h"
#include "scene_debug.h"
#include "../HAL/drawing.h"
#include "../utils/sprites.h"
#include "../utils/anim.h"
#include "../utils/button.h"
#include "scene_mainmenu.h"
#include "scene_searchpaczkomat.h"
#include "scene_init.h"
#include "../utils/lang.h"

static u64 lastTick = 0;
static float dt = 0.0f;
static float dtScale = 1.0f;
static float spinnerAngle = 0.0f;
static float spinnerRadius = 20.0f;
static float spinnerSpeed = 180.0f;

static Ease flash_ease;
static Ease button_ease;
static Ease slide_out_ease;
static Ease details_in_ease;

static Ease confirm_ease_in;
static Ease pay_ease_in;
static Ease va_msg_ease_in;
static bool pay_triggered = false;

bool ceny_parsed = false;
static bool confirm_parsed = false;
static bool nadaj_music_playing = false;

static bool wysylka_request_sent = false;
static Button paczka_sizeA_btn;
static Button paczka_sizeB_btn;
static Button paczka_sizeC_btn;
static Button paczka_sizeD_btn;

static Button tf_imie_btn;
static Button tf_nazwisko_btn;
static Button tf_phone_btn;
static Button tf_email_btn;

static GFX_TEXTBUF nadajBuf;
static GFX_TEXT text_priceA;
static GFX_TEXT text_priceB;
static GFX_TEXT text_priceC;
static GFX_TEXT text_priceD;

static GFX_TEXT text_detailsA;
static GFX_TEXT text_detailsB;
static GFX_TEXT text_detailsC;
static GFX_TEXT text_detailsD;

static GFX_TEXT text_titleA;
static GFX_TEXT text_titleB;
static GFX_TEXT text_titleC;
static GFX_TEXT text_titleD;

static GFX_TEXT text_tf_imie;
static GFX_TEXT text_tf_nazwisko;
static GFX_TEXT text_tf_phone;
static GFX_TEXT text_tf_email;
static GFX_TEXT A_text;

static GFX_TEXT text_size_select;
static GFX_TEXT text_details;

static GFX_TEXT text_confirm_prompt;
static GFX_TEXT text_confirm_details;
static GFX_TEXT text_confirm_hint;
static GFX_TEXT text_blik_prompt;
static GFX_TEXT text_blik_btn;
static GFX_TEXT text_va_msg;

bool phone_inputed, email_inputed, imie_inputed, nazwisko_inputed = false;

typedef enum {
    NADAJ_INIT = 0,
    NADAJ_SIZE_SELECT,   
    NADAJ_DETAILS,
    NADAJ_SEARCH_PACZKOMAT,
    NADAJ_PROMPT_CONFIRM,
    NADAJ_PAY,
    NADAJ_PAYMENT_CHECKER,
    NADAJ_BLIK_CHECKER,
    NADAJ_VA_MESSAGE,
    NADAJ_DONE,
    NADAJ_STATE_ERROR
} NadajState;

static NadajState current_nadaj_state = NADAJ_INIT;
static int va_offset;
static float va_timer = 0.0f;
static bool va_next_played = false;

void sceneNadajInit(void) {
    reciever_info.parcelPaid = false;
    bool returning_from_elsewhere = (phone_inputed && imie_inputed && nazwisko_inputed && email_inputed);
    wysylka_request_sent = false;
    stopCwav(1);
    
    switch(VOICEACT) {
        case 0:
            break;
        default:
            for (int i = 0; i < 7; i++) {
                getCwav(85 + ((VOICEACT - 1) * 7) + i);
            }
            break;
    }
    if (returning_to_nadaj) {
        if (current_nadaj_state < NADAJ_PROMPT_CONFIRM) {
            current_nadaj_state = NADAJ_PROMPT_CONFIRM;
        }
    } else {
        current_nadaj_state = NADAJ_INIT;
    }

    musicisplaying = false;

    if (!returning_to_nadaj && !nadaj_music_playing) {
        CWAV* nadaj_bgm = getCwav(84);
        nadaj_bgm->volume = 0.60f;
        playCwav(84, true);
        nadaj_music_playing = true;
    }
    
    flash_ease = initEase(1.2f, EASE_OUT);
    button_ease = initEase(1.2f, EASE_OUT);
    
    lastTick = svcGetSystemTick();
    dt = 0.0f;
    dtScale = 1.0f;
    
    if (!ceny_parsed) {
        cena_paczkas.done = false;
        getWysylkaCena();
    }
    
    Create_button(&paczka_sizeA_btn, -320.0f, 7.0f, 311, 54, paczka_size, paczka_size_selected, GFX_COLOR_RGBA(255,255,255,0), GFX_COLOR_RGBA(255,255,255,0), 1.0f);
    Create_button(&paczka_sizeB_btn, -320.0f, 64.0f, 311, 54, paczka_size, paczka_size_selected, GFX_COLOR_RGBA(255,255,255,0), GFX_COLOR_RGBA(255,255,255,0), 1.0f);
    Create_button(&paczka_sizeC_btn, -320.0f, 121.0f, 311, 54, paczka_size, paczka_size_selected, GFX_COLOR_RGBA(255,255,255,0), GFX_COLOR_RGBA(255,255,255,0), 1.0f);
    Create_button(&paczka_sizeD_btn, -320.0f, 178.0f, 311, 54, paczka_size, paczka_size_selected, GFX_COLOR_RGBA(255,255,255,0), GFX_COLOR_RGBA(255,255,255,0), 1.0f);
    
    Create_button(&tf_imie_btn, 400.0f, 7.0f, 300, 40, paczka_size, paczka_size_selected, GFX_COLOR_RGBA(255,255,255,0), GFX_COLOR_RGBA(255,255,255,0), 1.0f);
    Create_button(&tf_nazwisko_btn, 400.0f, 64.0f, 300, 40, paczka_size, paczka_size_selected, GFX_COLOR_RGBA(255,255,255,0), GFX_COLOR_RGBA(255,255,255,0), 1.0f);
    Create_button(&tf_phone_btn, 400.0f, 121.0f, 300, 40, paczka_size, paczka_size_selected, GFX_COLOR_RGBA(255,255,255,0), GFX_COLOR_RGBA(255,255,255,0), 1.0f);
    Create_button(&tf_email_btn, 400.0f, 178.0f, 300, 40, paczka_size, paczka_size_selected, GFX_COLOR_RGBA(255,255,255,0), GFX_COLOR_RGBA(255,255,255,0), 1.0f);
    
    nadajBuf = GFX_TextBufNew(2048);
    
    GFX_TextParse(&text_size_select, nadajBuf, lang_strings[14]);
    GFX_TextOptimize(&text_size_select);
    GFX_TextParse(&text_details, nadajBuf, lang_strings[15]);
    GFX_TextOptimize(&text_details);

    GFX_TextParse(&text_tf_imie, nadajBuf, imie_inputed ? reciever_info.firstName : lang_strings[16]);
    GFX_TextOptimize(&text_tf_imie);
    GFX_TextParse(&text_tf_nazwisko, nadajBuf, nazwisko_inputed ? reciever_info.lastName : lang_strings[17]);
    GFX_TextOptimize(&text_tf_nazwisko);
    GFX_TextParse(&text_tf_email, nadajBuf, email_inputed ? reciever_info.email : lang_strings[18]);
    GFX_TextOptimize(&text_tf_email);
    GFX_TextParse(&text_tf_phone, nadajBuf, phone_inputed ? reciever_info.phoneNumber : lang_strings[19]);
    GFX_TextOptimize(&text_tf_phone);
    GFX_TextParse(&A_text, nadajBuf, lang_strings[20]);
    GFX_TextOptimize(&A_text);

    GFX_TextParse(&text_confirm_prompt, nadajBuf, lang_strings[21]);
    GFX_TextOptimize(&text_confirm_prompt);
    GFX_TextParse(&text_confirm_hint, nadajBuf, lang_strings[22]);
    GFX_TextOptimize(&text_confirm_hint);
    GFX_TextParse(&text_blik_prompt, nadajBuf, lang_strings[23]);
    GFX_TextOptimize(&text_blik_prompt);
    GFX_TextParse(&text_blik_btn, nadajBuf, lang_strings[24]);
    GFX_TextOptimize(&text_blik_btn);
    GFX_TextParse(&text_va_msg, nadajBuf, lang_strings[25]);
    GFX_TextOptimize(&text_va_msg);

    confirm_parsed = false;
    pay_triggered = false;
    va_timer = 0.0f;
    va_next_played = false;
}

static bool prevTouchHeld = false;
static float cached_touch_x = 0.0f;
static float cached_touch_y = 0.0f;

Thread check_thread;
bool check_thread_already_run = false;
bool check_running = false;
typedef enum {
    CHECKER_INIT = 0,
    CHECKER_REQUEST,
    CHECKER_WAIT,
    CHECKER_PARSE,
    CHECKER_DONE
} CheckerState;

static CheckerState stan_checker = CHECKER_INIT;

void Checker_Routine(void* arg) {
    bool check_running = true;
    while (check_running) {
        switch (stan_checker) {
            case CHECKER_INIT:
                status_wysylki.done = false;
                stan_checker = CHECKER_REQUEST;
                break;
            case CHECKER_REQUEST:
                if (!fake_paczka_mode) {
                    checkWysylkaState(reciever_info);
                } else {
                    status_wysylki.done = true;
                }
                stan_checker = CHECKER_WAIT;
                break;
            case CHECKER_WAIT:
                svcSleepThread(3 * 1000000000LL);
                stan_checker = CHECKER_PARSE;
                break;
            case CHECKER_PARSE:
                if (status_wysylki.done) {
                    if (!fake_paczka_mode) {
                        parseWysylkaState(status_wysylki.data);
                    } else {
                        parseWysylkaState("\{\"status\":\"AUTHORIZED\"}");
                    }
                    if (!reciever_info.parcelPaid) {
                        stan_checker = CHECKER_REQUEST;
                    } else {
                        stan_checker = CHECKER_DONE;
                    }
                } else {
                    stan_checker = CHECKER_WAIT;
                }
                break;
            case CHECKER_DONE:
                check_running = false;
                break;
        }
    }
}

void start_CheckerThread() {
    stan_checker = CHECKER_INIT;
    if (!check_thread_already_run) {
        check_thread = threadCreate(Checker_Routine, NULL, 32 * 1024, 0x30, -2, false);
    }
    check_thread_already_run = true;
}

void stop_CheckerThread() {
    threadJoin(check_thread, UINT64_MAX);
    threadFree(check_thread);
    check_thread = NULL;
    check_thread_already_run = false;
}

void sceneNadajUpdate(uint32_t kDown, uint32_t kHeld) {
    updateEase(&flash_ease);
    u64 currentTick = svcGetSystemTick();
    dt = (float)(currentTick - lastTick) / CPU_TICKS_PER_MSEC / 1000.0f;
    lastTick = currentTick;

    if (dt > 0.1f) dt = 0.1f;
    if (dt < 0.001f) dt = 0.001f;

    dtScale = dt * 60.0f;
    
    if (cena_paczkas.done && !ceny_parsed) {
        parseWysylkaCena(cena_paczkas.data);
        if (VOICEACT != 0) {
            playCwav(85 + ((VOICEACT - 1) * 7), false);
        }
        char buf[128];

        GFX_TextParse(&text_titleA, nadajBuf, lang_strings[26]);
        GFX_TextOptimize(&text_titleA);
        GFX_TextParse(&text_titleB, nadajBuf, lang_strings[27]);
        GFX_TextOptimize(&text_titleB);
        GFX_TextParse(&text_titleC, nadajBuf, lang_strings[28]);
        GFX_TextOptimize(&text_titleC);
        GFX_TextParse(&text_titleD, nadajBuf, lang_strings[29]);
        GFX_TextOptimize(&text_titleD);

        snprintf(buf, sizeof(buf), "%.2fzł", parcelA_price);
        GFX_TextParse(&text_priceA, nadajBuf, buf);
        GFX_TextOptimize(&text_priceA);
        snprintf(buf, sizeof(buf), "%.2fzł", parcelB_price);
        GFX_TextParse(&text_priceB, nadajBuf, buf);
        GFX_TextOptimize(&text_priceB);
        snprintf(buf, sizeof(buf), "%.2fzł", parcelC_price);
        GFX_TextParse(&text_priceC, nadajBuf, buf);
        GFX_TextOptimize(&text_priceC);
        snprintf(buf, sizeof(buf), "%.2fzł", parcelD_price);
        GFX_TextParse(&text_priceD, nadajBuf, buf);
        GFX_TextOptimize(&text_priceD);
        
        snprintf(buf, sizeof(buf), lang_strings[25], parcelA_height, parcelA_width, parcelA_length, parcelA_weight);
        GFX_TextParse(&text_detailsA, nadajBuf, buf);
        GFX_TextOptimize(&text_detailsA);
        snprintf(buf, sizeof(buf), lang_strings[25], parcelB_height, parcelB_width, parcelB_length, parcelB_weight);
        GFX_TextParse(&text_detailsB, nadajBuf, buf);
        GFX_TextOptimize(&text_detailsB);
        snprintf(buf, sizeof(buf), lang_strings[25], parcelC_height, parcelC_width, parcelC_length, parcelC_weight);
        GFX_TextParse(&text_detailsC, nadajBuf, buf);
        GFX_TextOptimize(&text_detailsC);
        snprintf(buf, sizeof(buf), lang_strings[25], parcelD_height, parcelD_width, parcelD_length, parcelD_weight);
        GFX_TextParse(&text_detailsD, nadajBuf, buf);
        GFX_TextOptimize(&text_detailsD);
        ceny_parsed = true;
        
        if (current_nadaj_state == NADAJ_INIT && !phone_inputed) {
            current_nadaj_state = NADAJ_SIZE_SELECT;
        }
    }
    
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

    switch (current_nadaj_state) {
        case NADAJ_INIT:
            if (ceny_parsed) {
                current_nadaj_state = NADAJ_SIZE_SELECT;
            }
            break;

        case NADAJ_SIZE_SELECT:
            if (kDown & KEY_B) {
                playCwav(3, true);
                stopCwav(84);
                nadaj_music_playing = false;
                current_nadaj_state = NADAJ_INIT;
                phone_inputed = false;
                email_inputed = false;
                imie_inputed = false;
                nazwisko_inputed = false;
                sceneManagerSwitchTo(SCENE_SEND);
                break;
            }
            updateEase(&button_ease);
            updateEase(&details_in_ease);

            float current_btn_x = -320.0f + 323.0f * button_ease.value;
            
            paczka_sizeA_btn.x = current_btn_x;
            paczka_sizeB_btn.x = current_btn_x;
            paczka_sizeC_btn.x = current_btn_x;
            paczka_sizeD_btn.x = current_btn_x;

            float in_x = 30.0f + 390.0f * details_in_ease.value;
            tf_imie_btn.x = in_x;
            tf_nazwisko_btn.x = in_x;
            tf_email_btn.x = in_x;
            tf_phone_btn.x = in_x;

            Update_button(&paczka_sizeA_btn, touch, touchDown, touchHeld, touchUp);
            Update_button(&paczka_sizeB_btn, touch, touchDown, touchHeld, touchUp);
            Update_button(&paczka_sizeC_btn, touch, touchDown, touchHeld, touchUp);
            Update_button(&paczka_sizeD_btn, touch, touchDown, touchHeld, touchUp);

            if (paczka_sizeA_btn.clicked || paczka_sizeB_btn.clicked || paczka_sizeC_btn.clicked || paczka_sizeD_btn.clicked) {
                if (paczka_sizeA_btn.clicked) reciever_info.parcelSize = 1;
                if (paczka_sizeB_btn.clicked) reciever_info.parcelSize = 2;
                if (paczka_sizeC_btn.clicked) reciever_info.parcelSize = 3;
                if (paczka_sizeD_btn.clicked) reciever_info.parcelSize = 4;
                playCwav(4, true);
                if (VOICEACT != 0) {
                    playCwav(86 + ((VOICEACT - 1) * 7), false);
                }
                
                current_nadaj_state = NADAJ_DETAILS;
                slide_out_ease = initEase(0.8f, EASE_IN);
                details_in_ease = initEase(0.8f, EASE_OUT);
            }
            break;

        case NADAJ_DETAILS:
            if (kDown & KEY_B) {
                playCwav(3, true);
                current_nadaj_state = NADAJ_SIZE_SELECT;
                button_ease = initEase(0.8f, EASE_OUT);
                details_in_ease = initEase(0.8f, EASE_IN);
                break;
            }
            updateEase(&slide_out_ease);
            updateEase(&details_in_ease);

            float out_x = 3.0f - (323.0f * slide_out_ease.value);
            paczka_sizeA_btn.x = out_x;
            paczka_sizeB_btn.x = out_x;
            paczka_sizeC_btn.x = out_x;
            paczka_sizeD_btn.x = out_x;

            float details_x = 420.0f - (390.0f * details_in_ease.value);
            tf_imie_btn.x = details_x;
            tf_nazwisko_btn.x = details_x;
            tf_email_btn.x = details_x;
            tf_phone_btn.x = details_x;

            Update_button(&tf_imie_btn, touch, touchDown, touchHeld, touchUp);
            Update_button(&tf_nazwisko_btn, touch, touchDown, touchHeld, touchUp);
            Update_button(&tf_email_btn, touch, touchDown, touchHeld, touchUp);
            Update_button(&tf_phone_btn, touch, touchDown, touchHeld, touchUp);

            if (tf_imie_btn.clicked) {
                char mybuf[256];
                SwkbdState swkbd;
                SwkbdButton button = SWKBD_BUTTON_NONE;

                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, 59);
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

                button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));

                GFX_TextParse(&text_tf_imie, nadajBuf, mybuf);
                GFX_TextOptimize(&text_tf_imie);
                strncpy(reciever_info.firstName, mybuf, sizeof(reciever_info.firstName) - 1);
                imie_inputed = true;
            }
            if (tf_nazwisko_btn.clicked) {
                char mybuf[256];
                SwkbdState swkbd;
                SwkbdButton button = SWKBD_BUTTON_NONE;

                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, 59);
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

                button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));

                GFX_TextParse(&text_tf_nazwisko, nadajBuf, mybuf);
                GFX_TextOptimize(&text_tf_nazwisko);
                strncpy(reciever_info.lastName, mybuf, sizeof(reciever_info.lastName) - 1);
                nazwisko_inputed = true;
            }
            if (tf_email_btn.clicked) {
                char mybuf[256];
                SwkbdState swkbd;
                SwkbdButton button = SWKBD_BUTTON_NONE;

                swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, 59);
                swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

                button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));

                GFX_TextParse(&text_tf_email, nadajBuf, mybuf);
                GFX_TextOptimize(&text_tf_email);
                strncpy(reciever_info.email, mybuf, sizeof(reciever_info.email) - 1);
                email_inputed = true;
            }
            if (tf_phone_btn.clicked) {
                char mybuf[256];
                SwkbdState swkbd;
                SwkbdButton button = SWKBD_BUTTON_NONE;
                swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 9);
                swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
                swkbdSetValidation(&swkbd, SWKBD_FIXEDLEN, 0, 0);
                swkbdSetFeatures(&swkbd, SWKBD_FIXED_WIDTH);
                button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
                GFX_TextParse(&text_tf_phone, nadajBuf, mybuf);
                GFX_TextOptimize(&text_tf_phone);
                strncpy(reciever_info.phoneNumber, mybuf, sizeof(reciever_info.phoneNumber) - 1);
                phone_inputed = true;
            }
            if ((kDown & KEY_A) && (phone_inputed && email_inputed && nazwisko_inputed && imie_inputed)) {
                sceneManagerSwitchTo(SCENE_SEARCHPACZKOMAT);
                current_nadaj_state = NADAJ_PROMPT_CONFIRM;
                if (VOICEACT != 0) {
                    playCwav(87 + ((VOICEACT - 1) * 7), false);
                }
            }
            break;

        case NADAJ_PROMPT_CONFIRM:
            if (!confirm_parsed) {
                char buf[512];
                char sizeChar = 'A';
                if (reciever_info.parcelSize == 1) sizeChar = 'A';
                if (reciever_info.parcelSize == 2) sizeChar = 'B';
                if (reciever_info.parcelSize == 3) sizeChar = 'C';
                if (reciever_info.parcelSize == 4) sizeChar = 'D';

                snprintf(buf, sizeof(buf), lang_strings[68],
                         reciever_info.firstName, reciever_info.lastName, reciever_info.phoneNumber, 
                         reciever_info.email, reciever_info.boxMachineName, sizeChar);
                
                GFX_TextParse(&text_confirm_details, nadajBuf, buf);
                GFX_TextOptimize(&text_confirm_details);
                
                confirm_parsed = true;
                confirm_ease_in = initEase(0.8f, EASE_OUT);
                if (VOICEACT != 0) {
                    playCwav(88 + ((VOICEACT - 1) * 7), false);
                }
            }

            updateEase(&confirm_ease_in);
            
            if (!wysylka_request_sent) {
                if (kDown & KEY_A) {
                    playCwav(4, true);
                    if (!fake_paczka_mode) {
                        submitWysylkaRequest(reciever_info);
                    } else {
                        wysylka_request.done = true;
                    }
                    wysylka_request_sent = true;

                } else if (kDown & KEY_B) {
                    playCwav(3, true);
                    current_nadaj_state = NADAJ_DETAILS;
                    details_in_ease = initEase(0.8f, EASE_OUT);
                    confirm_parsed = false;
                }
            }

            if (wysylka_request.done) {
                if (!fake_paczka_mode) {
                    parseWysylkaRequest(wysylka_request.data);
                } else {
                    parseWysylkaRequest("\{\"shipmentNumber\":\"623333333333333333333333\",\"orderId\":\"7a7b66fc-cbxx-4e72-9466-3b9xx7bab3xx\",\"merchantName\":\"Nadanie Paczki\",\"posId\":\"V01xxxxxxxxxxxx\",\"prices\":{\"basePrice\":\"16.49\",\"finalPrice\":\"16.49\",\"currency\":\"PLN\"}}\n");
                }
                current_nadaj_state = NADAJ_PAY;
                pay_ease_in = initEase(0.8f, EASE_OUT);
                pay_triggered = false;
                if (VOICEACT != 0) {
                    playCwav(89 + ((VOICEACT - 1) * 7), false);
                }
            }
            break;
            
        case NADAJ_PAY:
            updateEase(&pay_ease_in);
            if (pay_ease_in.value >= 1.0f && !pay_triggered) {
                pay_triggered = true;
                playCwav(4, true);
                char mybuf[256];
                SwkbdState swkbd;
                SwkbdButton button = SWKBD_BUTTON_NONE;

                swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 6);
                swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
                swkbdSetValidation(&swkbd, SWKBD_FIXEDLEN, 6, 6);
                swkbdSetFeatures(&swkbd, SWKBD_FIXED_WIDTH);
                
                button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
                strncpy(
                    reciever_info.blikCode,
                    mybuf,
                    sizeof(reciever_info.blikCode) - 1
                );

                reciever_info.blikCode[
                    sizeof(reciever_info.blikCode) - 1
                ] = '\0';
                if (button == SWKBD_BUTTON_LEFT || button == SWKBD_BUTTON_NONE) {
                    current_nadaj_state = NADAJ_PROMPT_CONFIRM;
                    confirm_parsed = false;
                } else {
                    current_nadaj_state = NADAJ_PAYMENT_CHECKER;
                    if (!fake_paczka_mode) {
                        payBLIK();
                    } else {
                        blik_pay.done = true;
                    }
                }
            }
            break;

        case NADAJ_PAYMENT_CHECKER:
            if (blik_pay.done) {
                if (response_code == 400)  {
                    current_nadaj_state = NADAJ_PROMPT_CONFIRM;
                    confirm_parsed = false;
                } else if (response_code == 200) {
                    start_CheckerThread();
                    current_nadaj_state = NADAJ_BLIK_CHECKER;
                }
            }
            break;

        case NADAJ_BLIK_CHECKER:
            if (reciever_info.parcelPaid) {
                if (VOICEACT != 0) {
                    playCwav(90 + ((VOICEACT - 1) * 7), false);
                }
                stop_CheckerThread();
                va_timer = 0.0f;
                va_next_played = false;
                current_nadaj_state = NADAJ_VA_MESSAGE;
            }
            break;

        case NADAJ_VA_MESSAGE:
            va_timer += dt;
            if (va_timer >= 1.6f && !va_next_played) {
                if (VOICEACT != 0) {
                    playCwav(91 + ((VOICEACT - 1) * 7), false);
                }
                va_msg_ease_in = initEase(0.8f, EASE_OUT);
                va_next_played = true;
            }
            if (va_next_played) {
                updateEase(&va_msg_ease_in);
            }
            if (va_timer >= 11.6f) {
                stopCwav(84);
                nadaj_music_playing = false;
                current_nadaj_state = NADAJ_DONE;
                phone_inputed = false;
                email_inputed = false;
                imie_inputed = false;
                nazwisko_inputed = false;
                sceneManagerSwitchTo(SCENE_SEND);
            }
            break;

        default:
            break;
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
            GFX_DrawImageAt(kun_idle, -250.0f + (260.0f * flash_ease.value), 0.0f, 0.6f, NULL, 1.0f, 1.0f);
            break;
        case 2:
            GFX_DrawImageAt(chan_idle, -250.0f + (260.0f * flash_ease.value), 0.0f, 0.6f, NULL, 1.0f, 1.0f);
            break;  
        case 3:
            GFX_DrawImageAt(arhn_explain, -250.0f + (260.0f * flash_ease.value), 0.0f, 0.6f, NULL, 1.0f, 1.0f);
            break;  
        default: 
            GFX_DrawImageAt(chan_idle, -250.0f + (260.0f * flash_ease.value), 0.0f, 0.6f, NULL, 1.0f, 1.0f);
            break;  
    }

    if (current_nadaj_state == NADAJ_SIZE_SELECT || current_nadaj_state == NADAJ_INIT) {
        GFX_DrawShadowedText(
            &text_size_select,
            500 - (240.0f * flash_ease.value),
            50.0f,
            0.6f,
            0.75f,
            0.75f, GFX_ALIGN_CENTER,
            GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff),
            GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff)
        );
    } else if (current_nadaj_state == NADAJ_DETAILS) {
        GFX_DrawShadowedText(
            &text_details,
            500 - (240.0f * flash_ease.value),
            50.0f,
            0.6f,
            0.75f,
            0.75f, GFX_ALIGN_CENTER,
            GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff),
            GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff)
        );
        GFX_DrawShadowedText(
            &A_text,
            200,
            265.0f - (50.0f * details_in_ease.value),
            0.6f,
            0.7f,
            0.7f, GFX_ALIGN_CENTER,
            GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff),
            GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff)
        );
    } else if (current_nadaj_state == NADAJ_PROMPT_CONFIRM) {
        GFX_DrawShadowedText(
            &text_confirm_prompt,
            500.0f - (235.0f * confirm_ease_in.value),
            50.0f,
            0.6f, 0.65f, 0.65f, GFX_ALIGN_CENTER,
            GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff),
            GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff)
        );
    } else if (current_nadaj_state == NADAJ_PAY) {
        GFX_DrawShadowedText(
            &text_blik_prompt,
            500.0f - (235.0f * pay_ease_in.value),
            50.0f,
            0.6f, 0.75f, 0.75f, GFX_ALIGN_CENTER,
            GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff),
            GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff)
        );
    } else if (current_nadaj_state == NADAJ_VA_MESSAGE && va_next_played) {
        GFX_DrawShadowedText(
            &text_va_msg,
            500.0f - (235.0f * va_msg_ease_in.value),
            50.0f,
            0.6f, 0.60f, 0.60f, GFX_ALIGN_CENTER,
            GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff),
            GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff)
        );
    }

    GFX_DrawRectangle(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f));
}

void sceneNadajRender(void) {
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
    if (!ceny_parsed) {
        drawSpinner(160.0f, 120.0f, 255.0f);
    }
    if (current_nadaj_state == NADAJ_PAYMENT_CHECKER || current_nadaj_state == NADAJ_BLIK_CHECKER) {
        drawSpinner(160.0f, 120.0f, 255.0f);
    }
    GFX_DrawRectangle(0, 0, 1.0f, 400, 240, GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f), GFX_COLOR_RGBA(255,255,255,255 - flash_ease.value * 255.0f));
    
    if (ceny_parsed) {
        if (paczka_sizeA_btn.x > -310.0f) {
            Draw_button(&paczka_sizeA_btn);
            GFX_DrawText(&text_titleA, paczka_sizeA_btn.x + 10.0f, 12.0f, 1.0f, 0.8f, 0.8f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            GFX_DrawText(&text_detailsA, paczka_sizeA_btn.x + 10.0f, 36.0f, 1.0f, 0.45f, 0.45f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            GFX_DrawText(&text_priceA, paczka_sizeA_btn.x + 196.0f, 18.0f, 1.0f, 1.0f, 1.0f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            
            Draw_button(&paczka_sizeB_btn);
            GFX_DrawText(&text_titleB, paczka_sizeB_btn.x + 10.0f, 69.0f, 1.0f, 0.8f, 0.8f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            GFX_DrawText(&text_detailsB, paczka_sizeB_btn.x + 10.0f, 93.0f, 1.0f, 0.45f, 0.45f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            GFX_DrawText(&text_priceB, paczka_sizeB_btn.x + 196.0f, 75.0f, 1.0f, 1.0f, 1.0f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            
            Draw_button(&paczka_sizeC_btn);
            GFX_DrawText(&text_titleC, paczka_sizeC_btn.x + 10.0f, 126.0f, 1.0f, 0.8f, 0.8f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            GFX_DrawText(&text_detailsC, paczka_sizeC_btn.x + 10.0f, 150.0f, 1.0f, 0.45f, 0.45f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            GFX_DrawText(&text_priceC, paczka_sizeC_btn.x + 196.0f, 132.0f, 1.0f, 1.0f, 1.0f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            
            Draw_button(&paczka_sizeD_btn);
            GFX_DrawText(&text_titleD, paczka_sizeD_btn.x + 10.0f, 183.0f, 1.0f, 0.8f, 0.8f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            GFX_DrawText(&text_detailsD, paczka_sizeD_btn.x + 10.0f, 207.0f, 1.0f, 0.45f, 0.45f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            GFX_DrawText(&text_priceD, paczka_sizeD_btn.x + 196.0f, 189.0f, 1.0f, 1.0f, 1.0f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
        }

        if (current_nadaj_state == NADAJ_DETAILS || tf_imie_btn.x < 400.0f) {
            Draw_button(&tf_imie_btn);
            if (!imie_inputed) {
                GFX_DrawText(&text_tf_imie, tf_imie_btn.x + 10.0f, tf_imie_btn.y + 10.0f, 1.0f, 0.6f, 0.6f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(128, 128, 128, 255));
            } else {
                GFX_DrawText(&text_tf_imie, tf_imie_btn.x + 10.0f, tf_imie_btn.y + 12.0f, 1.0f, 0.8f, 0.8f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            }
            
            Draw_button(&tf_nazwisko_btn);
            if (!nazwisko_inputed) {
                GFX_DrawText(&text_tf_nazwisko, tf_nazwisko_btn.x + 10.0f, tf_nazwisko_btn.y + 10.0f, 1.0f, 0.6f, 0.6f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(128, 128, 128, 255));
            } else {
                GFX_DrawText(&text_tf_nazwisko, tf_nazwisko_btn.x + 10.0f, tf_nazwisko_btn.y + 12.0f, 1.0f, 0.8f, 0.8f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            }

            Draw_button(&tf_email_btn);
            if (!email_inputed) {
                GFX_DrawText(&text_tf_email, tf_email_btn.x + 10.0f, tf_email_btn.y + 10.0f, 1.0f, 0.6f, 0.6f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(128, 128, 128, 255));
            } else {
                GFX_DrawText(&text_tf_email, tf_email_btn.x + 10.0f, tf_email_btn.y + 12.0f, 1.0f, 0.8f, 0.8f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            }

            Draw_button(&tf_phone_btn);
            if (!phone_inputed) {
                GFX_DrawText(&text_tf_phone, tf_phone_btn.x + 10.0f, tf_phone_btn.y + 10.0f, 1.0f, 0.6f, 0.6f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(128, 128, 128, 255));
            } else {
                GFX_DrawText(&text_tf_phone, tf_phone_btn.x + 10.0f, tf_phone_btn.y + 12.0f, 1.0f, 0.8f, 0.8f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0, 0, 0, 255));
            }
        }
        
        if (current_nadaj_state == NADAJ_PROMPT_CONFIRM) {
            float details_x = -320.0f + 330.0f * confirm_ease_in.value;
            GFX_DrawShadowedText(&text_confirm_details, details_x + 30.0f, 20.0f, 1.0f, 0.6f, 0.6f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff), GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff));
            GFX_DrawShadowedText(&text_confirm_hint, details_x + 30.0f, 205.0f, 1.0f, 0.55f, 0.55f, GFX_ALIGN_LEFT, GFX_COLOR_RGBA(0xB1, 0xA2, 0x2F, 0xff), GFX_COLOR_RGBA(0xff, 0xff, 0xff, 0xff));
        } else if (current_nadaj_state == NADAJ_PAY) {
        }
    }
}

void sceneNadajExit(void) {
    if (current_nadaj_state == NADAJ_DONE) {
        stopCwav(84);
        nadaj_music_playing = false;
    }
}