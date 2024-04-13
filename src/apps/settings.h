#ifndef SETTINGS_APP_H
#define SETTINGS_APP_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void SETTINGS_init();
bool SETTINGS_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void SETTINGS_render();

#endif /* end of include guard: SETTINGS_H */
