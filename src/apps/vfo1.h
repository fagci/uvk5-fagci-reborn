#ifndef VFO1_APP_H
#define VFO1_APP_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void VFO1_init();
void VFO1_update();
bool VFO1_keyEx(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld,
                bool isProMode);
bool VFO1_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void VFO1_render();

extern bool gVfo1ProMode;

#endif /* end of include guard: VFO1_APP_H */
