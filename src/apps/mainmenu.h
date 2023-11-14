#ifndef MAINMENU_H
#define MAINMENU_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void MAINMENU_init();
void MAINMENU_update();
void MAINMENU_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void MAINMENU_render();

#endif /* end of include guard: MAINMENU_H */
