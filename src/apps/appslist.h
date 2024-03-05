#ifndef APPSLIST_H
#define APPSLIST_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdint.h>

void APPSLIST_init();
void APPSLIST_update();
bool APPSLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void APPSLIST_render();
App *APPSLIST_Meta();

#endif /* end of include guard: APPSLIST_H */
