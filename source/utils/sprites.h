
#ifndef SPRITES_H
#define SPRITES_H
#include <citro2d.h>
#include <stdlib.h>
#include "../HAL/data.h"



extern GFX_IMAGE* test;
// extern GFX_IMAGE* fridge_image;
extern GFX_IMAGE* bg_top;
extern GFX_IMAGE* bg_bottom;
extern GFX_IMAGE* logo3ds;



extern GFX_IMAGE* paczka_closed;
extern GFX_IMAGE* znaczek;
extern GFX_IMAGE* paczka_sel;
extern GFX_IMAGE* otworz_zdalnie_button;



extern GFX_IMAGE* chan_placeholder;
extern GFX_IMAGE* kun_placeholder;


extern GFX_IMAGE* arhn_siema;
extern GFX_IMAGE* arhn_explain;
extern GFX_IMAGE* arhn_diss;


extern GFX_IMAGE* chan_excited;
extern GFX_IMAGE* chan_idle_speak;
extern GFX_IMAGE* chan_idle;
extern GFX_IMAGE* chan_diss_speak;
extern GFX_IMAGE* chan_diss;


extern GFX_IMAGE* kun_excited;
extern GFX_IMAGE* kun_idle_speak;
extern GFX_IMAGE* kun_idle;
extern GFX_IMAGE* kun_diss_speak;
extern GFX_IMAGE* kun_diss;


extern GFX_IMAGE* sie_do_wiezienia;


extern GFX_IMAGE* tut0_0;
extern GFX_IMAGE* tut0_1;
extern GFX_IMAGE* tut0_2;
extern GFX_IMAGE* tut0_3;
extern GFX_IMAGE* tut0_bot0;
extern GFX_IMAGE* tut0_bot1;


extern GFX_IMAGE* tut1_0;
extern GFX_IMAGE* tut1_bot0;
extern GFX_IMAGE* tut1_bot1;


extern GFX_IMAGE* tut2_0;


extern GFX_IMAGE* tut3_0;

extern GFX_IMAGE* paczka_size;
extern GFX_IMAGE* paczka_size_selected;

extern GFX_IMAGE* credits_button;
extern GFX_IMAGE* refresh_button;
extern GFX_IMAGE* settings_button;

extern GFX_IMAGE* helper_qr;

extern GFX_IMAGE* title_1;
extern GFX_IMAGE* title_2;
extern GFX_IMAGE* title_3;
extern GFX_IMAGE* title_4;

void spritesInit();
#endif