#ifndef PRESETLIST_APP_H
#define PRESETLIST_APP_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdbool.h>
#include <stdint.h>

void PRESETLIST_init();
void PRESETLIST_update();
bool PRESETLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void PRESETLIST_render();
App *PRESETLIST_Meta(void);

#endif /* end of include guard: PRESETLIST_APP_H */
