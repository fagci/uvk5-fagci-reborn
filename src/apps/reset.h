#ifndef RESET_H
#define RESET_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"

void RESET_Init();
void RESET_Update();
void RESET_Render();
bool RESET_key(KEY_Code_t k, bool p, bool h);
App *RESET_Meta();

#endif /* end of include guard: RESET_H */
