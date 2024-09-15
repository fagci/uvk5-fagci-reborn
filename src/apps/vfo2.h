#ifndef VFO2_APP_H
#define VFO2_APP_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

bool VFO2_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void VFO2_render();

#endif /* end of include guard: VFO2_APP_H */
