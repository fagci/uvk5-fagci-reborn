#ifndef TEXTINPUT_H
#define TEXTINPUT_H

#include "../driver/keyboard.h"
#include "../helper/appsregistry.h"
#include <stdbool.h>
#include <stdint.h>

void TEXTINPUT_init();
void TEXTINPUT_update();
bool TEXTINPUT_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld);
void TEXTINPUT_render();
void TEXTINPUT_deinit();
App *TEXTINPUT_Meta(void);

extern char *gTextinputText;
extern uint8_t gTextInputSize;
extern void (*gTextInputCallback)(void);

#endif /* end of include guard: TEXTINPUT_H */
