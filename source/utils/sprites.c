#include "sprites.h"
#include <3ds.h>
#include <stdlib.h> 
#include "../main.h"


extern size_t linear_bytes_used;



GFX_IMAGE* test;
// GFX_IMAGE* fridge_image;
GFX_IMAGE* bg_top;
GFX_IMAGE* bg_bottom;
GFX_IMAGE* logo3ds;



GFX_IMAGE* paczka_closed;
GFX_IMAGE* znaczek;
GFX_IMAGE* paczka_sel;
GFX_IMAGE* otworz_zdalnie_button;



GFX_IMAGE* chan_placeholder;
GFX_IMAGE* kun_placeholder;


GFX_IMAGE* arhn_siema;
GFX_IMAGE* arhn_explain;
GFX_IMAGE* arhn_diss;


GFX_IMAGE* chan_excited;
GFX_IMAGE* chan_idle_speak;
GFX_IMAGE* chan_idle;
GFX_IMAGE* chan_diss_speak;
GFX_IMAGE* chan_diss;


GFX_IMAGE* kun_excited;
GFX_IMAGE* kun_idle_speak;
GFX_IMAGE* kun_idle;
GFX_IMAGE* kun_diss_speak;
GFX_IMAGE* kun_diss;


GFX_IMAGE* sie_do_wiezienia;


GFX_IMAGE* tut0_0;
GFX_IMAGE* tut0_1;
GFX_IMAGE* tut0_2;
GFX_IMAGE* tut0_3;
GFX_IMAGE* tut0_bot0;
GFX_IMAGE* tut0_bot1;


GFX_IMAGE* tut1_0;
GFX_IMAGE* tut1_bot0;
GFX_IMAGE* tut1_bot1;


GFX_IMAGE* tut2_0;


GFX_IMAGE* tut3_0;

GFX_IMAGE* paczka_size;
GFX_IMAGE* paczka_size_selected;

GFX_IMAGE* credits_button;
GFX_IMAGE* refresh_button;
GFX_IMAGE* settings_button;

GFX_IMAGE* helper_qr;

GFX_IMAGE* title_1;
GFX_IMAGE* title_2;
GFX_IMAGE* title_3;
GFX_IMAGE* title_4;

u64 lastFrameTime = 0;
int currentFrame = 0;


void spritesInit() {
    srand(osGetTime());

    
    
    s64 memBefore = osGetMemRegionUsed(MEMREGION_APPLICATION);

    // mamy logo to mi sie to nie przyda xdddd
    // if (rand() % 2 == 0) {
    //     fridge_image = GFX_LoadTexture("romfs:/gfx/tehfridge2.t3x", 0);
    // } else {
    //     fridge_image = GFX_LoadTexture("romfs:/gfx/tehfridge.t3x", 0);
    // }
    
    
    
    s64 memAfter = osGetMemRegionUsed(MEMREGION_APPLICATION);
    
    
    if (memAfter > memBefore) {
        linear_bytes_used += (size_t)(memAfter - memBefore);
    }
    
    sie_do_wiezienia = GFX_LoadTexture("romfs:/gfx/pakuj.t3x", 0);
    arhn_siema = GFX_LoadTexture("romfs:/gfx/arhn.t3x", 0);
    arhn_explain = GFX_LoadTexture("romfs:/gfx/arhn.t3x", 1);
    arhn_diss = GFX_LoadTexture("romfs:/gfx/arhn.t3x", 2);
    
    chan_excited = GFX_LoadTexture("romfs:/gfx/chan.t3x", 0);
    chan_idle_speak = GFX_LoadTexture("romfs:/gfx/chan.t3x", 1);
    chan_idle = GFX_LoadTexture("romfs:/gfx/chan.t3x", 2);
    chan_diss_speak = GFX_LoadTexture("romfs:/gfx/chan.t3x", 3);
    chan_diss = GFX_LoadTexture("romfs:/gfx/chan.t3x", 4);

    kun_excited = GFX_LoadTexture("romfs:/gfx/kun.t3x", 0);
    kun_idle_speak = GFX_LoadTexture("romfs:/gfx/kun.t3x", 1);
    kun_idle = GFX_LoadTexture("romfs:/gfx/kun.t3x", 2);
    kun_diss_speak = GFX_LoadTexture("romfs:/gfx/kun.t3x", 3);
    kun_diss = GFX_LoadTexture("romfs:/gfx/kun.t3x", 4);

    tut0_0 = GFX_LoadTexture("romfs:/gfx/tutorial_0.t3x", 0);
    tut0_1 = GFX_LoadTexture("romfs:/gfx/tutorial_0.t3x", 1);
    tut0_2 = GFX_LoadTexture("romfs:/gfx/tutorial_0.t3x", 2);
    tut0_3 = GFX_LoadTexture("romfs:/gfx/tutorial_0.t3x", 3);
    tut0_bot0 = GFX_LoadTexture("romfs:/gfx/tutorial_0_bottom.t3x", 0);
    tut0_bot1 = GFX_LoadTexture("romfs:/gfx/tutorial_0_bottom.t3x", 1);
    tut1_0 = GFX_LoadTexture("romfs:/gfx/tutorial_1.t3x", 0);
    tut1_bot0 = GFX_LoadTexture("romfs:/gfx/tutorial_1_bottom.t3x", 0);
    tut1_bot1 = GFX_LoadTexture("romfs:/gfx/tutorial_1_bottom.t3x", 1);
    tut2_0 = GFX_LoadTexture("romfs:/gfx/tutorial_2.t3x", 0);
    tut3_0 = GFX_LoadTexture("romfs:/gfx/tutorial_3.t3x", 0);
    chan_placeholder = GFX_LoadTexture("romfs:/gfx/placeholder_chan.t3x", 0);
    kun_placeholder = GFX_LoadTexture("romfs:/gfx/placeholder_chan.t3x", 1);
    bg_top = GFX_LoadTexture("romfs:/gfx/bg.t3x", 0);
    bg_bottom = GFX_LoadTexture("romfs:/gfx/bg.t3x", 1);
    logo3ds = GFX_LoadTexture("romfs:/gfx/logo_inpost.t3x", 0);
    paczka_closed = GFX_LoadTexture("romfs:/gfx/paczka.t3x", 0);
    znaczek = GFX_LoadTexture("romfs:/gfx/paczka.t3x", 2);
    paczka_sel = GFX_LoadTexture("romfs:/gfx/paczka.t3x", 1);
    otworz_zdalnie_button = GFX_LoadTexture("romfs:/gfx/paczka.t3x", 3);
    paczka_size = GFX_LoadTexture("romfs:/gfx/paczka_size.t3x", 0);
    paczka_size_selected = GFX_LoadTexture("romfs:/gfx/paczka_size.t3x", 1);
    credits_button = GFX_LoadTexture("romfs:/gfx/menu_btn.t3x", 1);
    refresh_button = GFX_LoadTexture("romfs:/gfx/menu_btn.t3x", 0);
    settings_button = GFX_LoadTexture("romfs:/gfx/menu_btn.t3x", 2);
    helper_qr = GFX_LoadTexture("romfs:/gfx/helper_qr.t3x", 0);

    title_1 = GFX_LoadTexture("romfs:/gfx/title_logo.t3x", 0);
    title_2 = GFX_LoadTexture("romfs:/gfx/title_logo.t3x", 1);
    title_3 = GFX_LoadTexture("romfs:/gfx/title_logo.t3x", 2);
    title_4 = GFX_LoadTexture("romfs:/gfx/title_logo.t3x", 3);
}