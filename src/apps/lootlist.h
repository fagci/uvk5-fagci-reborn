#ifndef LOOTLIST_APP_H
#define LOOTLIST_APP_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void LOOTLIST_init();
void LOOTLIST_update();
bool LOOTLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void LOOTLIST_render();

#endif /* end of include guard: LOOTLIST_APP_H */
