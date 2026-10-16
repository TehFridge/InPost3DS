
#ifndef SCENE_MAINMENU_H
#define SCENE_MAINMENU_H

extern bool musicisplaying;
void unloadVA();
void loadVA();
void sceneMainMenuInit(void);
void sceneMainMenuUpdate(uint32_t kDown, uint32_t kHeld);
void sceneMainMenuRender(void);
void sceneMainMenuExit(void);

#endif
