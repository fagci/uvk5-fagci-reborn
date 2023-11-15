#ifndef RESET_H
#define RESET_H

#include "../driver/keyboard.h"

void RESET_Init();
void RESET_Update();
void RESET_Render();
void RESET_Key(KEY_Code_t k, bool p, bool h);

#endif /* end of include guard: RESET_H */
