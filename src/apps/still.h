#ifndef STILL_H
#define STILL_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdint.h>

void STILL_init();
void STILL_update();
bool STILL_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void STILL_render();
void STILL_deinit();
App *STILL_Meta();

#endif /* end of include guard: STILL_H */
