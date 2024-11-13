#ifndef PRESETLIST_APP_H
#define PRESETLIST_APP_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void PRESETLIST_init();
bool PRESETLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void PRESETLIST_render();

#endif /* end of include guard: PRESETLIST_APP_H */
