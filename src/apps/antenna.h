#ifndef ANTENNA_H
#define ANTENNA_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdint.h>

void ANTENNA_init();
void ANTENNA_update();
bool ANTENNA_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void ANTENNA_render();
void ANTENNA_deinit();
App *ANTENNA_Meta();

#endif /* end of include guard: ANTENNA_H */
