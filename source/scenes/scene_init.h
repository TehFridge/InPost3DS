#ifndef SCENE_INIT_H
#define SCENE_INIT_H
#include <stdint.h>
extern int VOICEACT;

void sceneInitInit(void);
void sceneInitUpdate(uint32_t kDown, uint32_t kHeld);
void sceneInitRender(void);
void sceneInitExit(void);

#endif
