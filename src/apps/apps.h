#ifndef APPS_H
#define APPS_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include "../settings.h"

extern App *gCurrentApp;

App *APPS_Peek();
bool APPS_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void APPS_init(App *app);
void APPS_update();
void APPS_render();
void APPS_RunPure(App *app);
void APPS_run(AppType_t id);
void APPS_runManual(App *app);
bool APPS_exit();

#endif /* end of include guard: APPS_H */
