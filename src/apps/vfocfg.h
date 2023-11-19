#ifndef VFOCFG_H
#define VFOCFG_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void VFOCFG_init();
void VFOCFG_update();
bool VFOCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void VFOCFG_render();

#endif /* end of include guard: VFOCFG_H */
