#ifndef TEXTINPUT_H
#define TEXTINPUT_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>

void TEXTINPUT_init();
void TEXTINPUT_update();
bool TEXTINPUT_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void TEXTINPUT_render();

extern char *gTextinputText;

#endif /* end of include guard: TEXTINPUT_H */
