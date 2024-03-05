#ifndef CHANNELSCANNER_H
#define CHANNELSCANNER_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdint.h>
#include <string.h>

void CHSCANNER_init();
void CHSCANNER_deinit();
bool CHSCANNER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void CHSCANNER_update();
void CHSCANNER_render();
App *CHSCANNER_Meta();

#endif /* end of include guard: CHANNELSCANNER_H */
