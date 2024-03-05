#ifndef SETTINGS_APP_H
#define SETTINGS_APP_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdint.h>

void SETTINGS_init();
void SETTINGS_update();
bool SETTINGS_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void SETTINGS_render();
App *SETTINGS_Meta();

#endif /* end of include guard: SETTINGS_H */
