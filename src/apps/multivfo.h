#ifndef MULTIVFO_APP_H
#define MULTIVFO_APP_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdbool.h>
#include <stdint.h>

void MULTIVFO_init();
void MULTIVFO_update();
bool MULTIVFO_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void MULTIVFO_render();
void MULTIVFO_deinit();
App *MULTIVFO_Meta(void);

#endif /* end of include guard: MULTIVFO_APP_H */
