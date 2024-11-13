#ifndef LEVEL_H
#define LEVEL_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void LEVEL_init(void);
void LEVEL_deinit(void);
void LEVEL_update(void);
bool LEVEL_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void LEVEL_render(void);

#endif /* end of include guard: LEVEL_H */
