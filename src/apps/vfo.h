#ifndef VFO_APP_H
#define VFO_APP_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void VFO_init();
void VFO_update();
bool VFO_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void VFO_render();

#endif /* end of include guard: VFO_APP_H */
