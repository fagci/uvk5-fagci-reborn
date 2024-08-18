#ifndef ANTENNA_H
#define ANTENNA_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void ANTENNA_init();
bool ANTENNA_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void ANTENNA_render();
void ANTENNA_deinit();

#endif /* end of include guard: ANTENNA_H */
