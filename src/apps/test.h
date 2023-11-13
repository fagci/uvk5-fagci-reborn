#ifndef TEST_H
#define TEST_H

#include "../driver/keyboard.h"

void TEST_Init();
void TEST_Update();
void TEST_Render();
void TEST_Key(KEY_Code_t k, bool p, bool h);

#endif /* end of include guard: TEST_H */
