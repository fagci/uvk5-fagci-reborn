#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include "../radio.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

bool SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void SPECTRUM_init(void);
void SPECTRUM_deinit(void);
void SPECTRUM_update(void);
void SPECTRUM_render(void);
App *SPECTRUM_Meta(void);

#endif /* ifndef SPECTRUM_H */
