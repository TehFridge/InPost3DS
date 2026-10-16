#ifndef SCENE_NADAJ_H
#define SCENE_NADAJ_H
#include <stdint.h>

void sceneNadajInit(void);
void sceneNadajUpdate(uint32_t kDown, uint32_t kHeld);
void sceneNadajRender(void);
void sceneNadajExit(void);

#endif
