#ifndef BANDCFG_H
#define BANDCFG_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

extern uint8_t bandCfgIndex;

void BANDCFG_init();
void BANDCFG_update();
bool BANDCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void BANDCFG_render();

#endif /* end of include guard: BANDCFG_H */
