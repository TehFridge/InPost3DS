#ifndef SCENE_TUTORIAL_H
#define SCENE_TUTORIAL_H
#include <stdint.h>

void sceneTutorialInit(void);
void sceneTutorialUpdate(uint32_t kDown, uint32_t kHeld);
void sceneTutorialRender(void);
void sceneTutorialExit(void);

#endif
