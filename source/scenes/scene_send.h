#ifndef SCENE_SEND_H
#define SCENE_SEND_H
#include <stdint.h>

void sceneSendInit(void);
void sceneSendUpdate(uint32_t kDown, uint32_t kHeld);
void sceneSendRender(void);
void sceneSendExit(void);

#endif
