#ifndef BANDCFG_H
#define BANDCFG_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdint.h>

void BANDCFG_init();
void BANDCFG_update();
bool BANDCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void BANDCFG_render();
App *BANDCFG_Meta();

#endif /* end of include guard: BANDCFG_H */
