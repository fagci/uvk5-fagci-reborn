#ifndef TASKMAN_H
#define TASKMAN_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void TASKMAN_Init(void);
void TASKMAN_Render(void);
bool TASKMAN_Key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);

#endif /* end of include guard: TASKMAN_H */
