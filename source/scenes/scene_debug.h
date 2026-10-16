#ifndef SCENE_DEBUG_H
#define SCENE_DEBUG_H
#include <stdint.h>
extern bool debugmode;
extern bool fake_paczka_mode;
void sceneDebugInit(void);
void sceneDebugUpdate(uint32_t kDown, uint32_t kHeld);
void sceneDebugRender(void);
void sceneDebugExit(void);

#endif
