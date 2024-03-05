#ifndef ANALYZER_H
#define ANALYZER_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include "../radio.h"
#include <stdint.h>
#include <string.h>

bool ANALYZER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void ANALYZER_init();
void ANALYZER_deinit();
void ANALYZER_update();
void ANALYZER_render();
App *ANALYZER_Meta();

#endif /* end of include guard: ANALYZER_H */
