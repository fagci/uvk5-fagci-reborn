#ifndef VFOPRO_H
#define VFOPRO_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void VFOPRO_init();
void VFOPRO_update();
bool VFOPRO_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void VFOPRO_render();
void VFOPRO_deinit();

#endif /* end of include guard: VFOPRO_H */
