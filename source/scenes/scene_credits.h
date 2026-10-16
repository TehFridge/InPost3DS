#ifndef SCENE_CREDITS_H
#define SCENE_CREDITS_H
#include <stdint.h>

void sceneCreditsInit(void);
void sceneCreditsUpdate(uint32_t kDown, uint32_t kHeld);
void sceneCreditsRender(void);
void sceneCreditsExit(void);

#endif
