#ifndef FINPUT_H
#define FINPUT_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void FINPUT_init();
bool FINPUT_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void FINPUT_render();

#endif /* end of include guard: FINPUT_H */
