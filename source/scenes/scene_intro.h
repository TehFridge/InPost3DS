#ifndef SCENE_INTRO_H
#define SCENE_INTRO_H
#include <stdint.h>

void sceneIntroInit(void);

void sceneIntroUpdate(uint32_t kDown, uint32_t kHeld);

void sceneIntroRender(void);

void sceneIntroExit(void);
#endif