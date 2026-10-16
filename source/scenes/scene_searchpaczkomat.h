#ifndef SCENE_SEARCHPACZKOMAT_H
#define SCENE_SEARCHPACZKOMAT_H
#include <stdint.h>
extern bool returning_to_nadaj;
void sceneSearchpaczkomatInit(void);
void sceneSearchpaczkomatUpdate(uint32_t kDown, uint32_t kHeld);
void sceneSearchpaczkomatRender(void);
void sceneSearchpaczkomatExit(void);

#endif
