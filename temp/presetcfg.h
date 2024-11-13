#ifndef PRESETCFG_H
#define PRESETCFG_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

extern uint8_t presetCfgIndex;

void PRESETCFG_init();
void PRESETCFG_update();
bool PRESETCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void PRESETCFG_render();

#endif /* end of include guard: PRESETCFG_H */
