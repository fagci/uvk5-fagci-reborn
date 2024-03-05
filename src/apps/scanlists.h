#ifndef SCANLISTS_H
#define SCANLISTS_H


#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdint.h>

void SCANLISTS_init();
void SCANLISTS_update();
bool SCANLISTS_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void SCANLISTS_render();
App *SCANLISTS_Meta();


#endif /* end of include guard: SCANLISTS_H */
