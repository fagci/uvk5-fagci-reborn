#ifndef STILL_H
#define STILL_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void STILL_init();
void STILL_update();
void STILL_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void STILL_render();

#endif /* end of include guard: STILL_H */
