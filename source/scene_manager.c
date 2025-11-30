#include "scene_manager.h"
#include "sprites.h"
#include "scene_intro.h"
#include "scene_mainmenu.h"


bool debug;
static SceneType currentScene = SCENE_NONE;

void sceneManagerInit(SceneType initialScene) {
    sceneManagerSwitchTo(initialScene);
}

void sceneManagerUpdate(uint32_t kDown, uint32_t kHeld) {
    switch (currentScene) {
        case SCENE_INTRO: sceneIntroUpdate(kDown, kHeld); break;
        case SCENE_MAINMENU: sceneMainmenuUpdate(kDown, kHeld); break;
        default: break;
    }
}

void sceneManagerRender(void) {
    switch (currentScene) {
        case SCENE_INTRO: sceneIntroRender(); break;
        case SCENE_MAINMENU: sceneMainmenuRender(); break;
        default: break;
    }
}

void sceneManagerSwitchTo(SceneType nextScene) {
    
    switch (currentScene) {
        case SCENE_INTRO: sceneIntroExit(); break;
        case SCENE_MAINMENU: sceneMainmenuExit(); break;
        default: break;
    }


    currentScene = nextScene;
    switch (currentScene) {
        case SCENE_INTRO: sceneIntroInit(); break;
        case SCENE_MAINMENU: sceneMainmenuInit(); break;
        default: break;
    }
}

void sceneManagerExit(void) {
    sceneManagerSwitchTo(SCENE_NONE);
}
