#ifndef BANDLIST_APP_H
#define BANDLIST_APP_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdint.h>

void BANDLIST_init();
void BANDLIST_update();
bool BANDLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void BANDLIST_render();
App *BANDLIST_Meta();

#endif /* end of include guard: BANDLIST_APP_H */
