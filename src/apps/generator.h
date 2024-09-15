#ifndef GENERATOR_H
#define GENERATOR_H


#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void GENERATOR_init();
void GENERATOR_update();
bool GENERATOR_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void GENERATOR_render();

#endif /* end of include guard: GENERATOR_H */

