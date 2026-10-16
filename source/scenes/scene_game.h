#ifndef SCENE_GAME_H
#define SCENE_GAME_H
#include <stdint.h>

void sceneGameInit(void);

void sceneGameUpdate(uint32_t kDown, uint32_t kHeld);

void sceneGameRender(void);

void sceneGameExit(void);
#endif