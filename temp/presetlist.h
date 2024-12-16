#ifndef BANDLIST_APP_H
#define BANDLIST_APP_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void BANDLIST_init();
bool BANDLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void BANDLIST_render();

#endif /* end of include guard: BANDLIST_APP_H */
