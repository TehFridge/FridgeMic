#ifndef SCENE_MAINMENU_H
#define SCENE_MAINMENU_H

#include <stdint.h>

void sceneMainmenuInit(void);
void sceneMainmenuUpdate(uint32_t kDown, uint32_t kHeld);
void sceneMainmenuRender(void);
void sceneMainmenuExit(void);

#endif
