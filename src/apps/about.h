#ifndef ABOUT_H
#define ABOUT_H

#include "../driver/keyboard.h"

void ABOUT_Init();
void ABOUT_Deinit();
void ABOUT_Update();
void ABOUT_Render();
bool ABOUT_key(KEY_Code_t k, bool p, bool h);

#endif /* end of include guard: ABOUT_H */
