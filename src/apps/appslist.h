#ifndef APPSLIST_H
#define APPSLIST_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void APPSLIST_init();
bool APPSLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void APPSLIST_render();

#endif /* end of include guard: APPSLIST_H */
