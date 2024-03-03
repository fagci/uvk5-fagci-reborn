#ifndef CHCFG_H
#define CHCFG_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdbool.h>
#include <stdint.h>

void CHCFG_init();
void CHCFG_update();
bool CHCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void CHCFG_render();
App *CHCFG_Meta(void);

#endif /* end of include guard: CHCFG_H */
