#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H
#include "scene_types.h"
#include <stdbool.h>
#include <stdint.h>

extern bool debug;
void sceneManagerInit(SceneType initialScene);
void sceneManagerUpdate(uint32_t kDown, uint32_t kHeld);
void sceneManagerRender(void);
void sceneManagerSwitchTo(SceneType nextScene);
void sceneManagerExit(void);

#endif
