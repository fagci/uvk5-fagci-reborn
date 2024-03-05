#ifndef SAVECH_H
#define SAVECH_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdint.h>

void SAVECH_init();
void SAVECH_update();
bool SAVECH_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void SAVECH_render();
App *SAVECH_Meta();

#endif /* end of include guard: SAVECH_H */
