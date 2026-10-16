#include "scene_manager.h"
#include "scene_mainmenu.h"
#include "scene_intro.h"
#include "scene_home.h"
#include "../utils/sprites.h"
#include "scene_debug.h"
#include "scene_tutorial.h"
#include "scene_init.h"
#include "scene_credits.h"
#include "scene_send.h"
#include "scene_nadaj.h"
#include "scene_searchpaczkomat.h"

bool debug;
static SceneType currentScene = SCENE_NONE;

void sceneManagerInit(SceneType initialScene) {
    sceneManagerSwitchTo(initialScene);
}

void sceneManagerUpdate(uint32_t kDown, uint32_t kHeld) {
    switch (currentScene) {
        case SCENE_MAIN_MENU: sceneMainMenuUpdate(kDown, kHeld); break;
        case SCENE_INTRO: sceneIntroUpdate(kDown, kHeld); break;
        case SCENE_HOME_MENU: sceneHomeMenuUpdate(kDown, kHeld); break;
        case SCENE_DEBUG: sceneDebugUpdate(kDown, kHeld); break;
        case SCENE_TUTORIAL: sceneTutorialUpdate(kDown, kHeld); break;
        case SCENE_INIT: sceneInitUpdate(kDown, kHeld); break;
        case SCENE_CREDITS: sceneCreditsUpdate(kDown, kHeld); break;
        case SCENE_SEND: sceneSendUpdate(kDown, kHeld); break;
        case SCENE_NADAJ: sceneNadajUpdate(kDown, kHeld); break;
        case SCENE_SEARCHPACZKOMAT: sceneSearchpaczkomatUpdate(kDown, kHeld); break;
        default: break;
    }
}

void sceneManagerRender(void) {
    switch (currentScene) {
        case SCENE_MAIN_MENU: sceneMainMenuRender(); break;
        case SCENE_INTRO: sceneIntroRender(); break;
        case SCENE_HOME_MENU: sceneHomeMenuRender(); break;
        case SCENE_DEBUG: sceneDebugRender(); break;
        case SCENE_TUTORIAL: sceneTutorialRender(); break;
        case SCENE_INIT: sceneInitRender(); break;
        case SCENE_CREDITS: sceneCreditsRender(); break;
        case SCENE_SEND: sceneSendRender(); break;
        case SCENE_NADAJ: sceneNadajRender(); break;
        case SCENE_SEARCHPACZKOMAT: sceneSearchpaczkomatRender(); break;
        default: break;
    }
}

void sceneManagerSwitchTo(SceneType nextScene) {
    
    switch (currentScene) {
        case SCENE_MAIN_MENU: sceneMainMenuExit(); break;
        case SCENE_INTRO: sceneIntroExit(); break;
        case SCENE_HOME_MENU: sceneHomeMenuExit(); break;
        case SCENE_DEBUG: sceneDebugExit(); break;
        case SCENE_TUTORIAL: sceneTutorialExit(); break;
        case SCENE_INIT: sceneInitExit(); break;
        case SCENE_CREDITS: sceneCreditsExit(); break;
        case SCENE_SEND: sceneSendExit(); break;
        case SCENE_NADAJ: sceneNadajExit(); break;
        case SCENE_SEARCHPACZKOMAT: sceneSearchpaczkomatExit(); break;
        default: break;
    }


    currentScene = nextScene;
    switch (currentScene) {
        case SCENE_MAIN_MENU: sceneMainMenuInit(); break;
        case SCENE_INTRO: sceneIntroInit(); break;
        case SCENE_HOME_MENU: sceneHomeMenuInit(); break;
        case SCENE_DEBUG: sceneDebugInit(); break;
        case SCENE_TUTORIAL: sceneTutorialInit(); break;
        case SCENE_INIT: sceneInitInit(); break;
        case SCENE_CREDITS: sceneCreditsInit(); break;
        case SCENE_SEND: sceneSendInit(); break;
        case SCENE_NADAJ: sceneNadajInit(); break;
        case SCENE_SEARCHPACZKOMAT: sceneSearchpaczkomatInit(); break;
        default: break;
    }
}

void sceneManagerExit(void) {
    sceneManagerSwitchTo(SCENE_NONE);
}
