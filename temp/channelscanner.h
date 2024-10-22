#ifndef CHANNELSCANNER_H
#define CHANNELSCANNER_H

#include "../driver/keyboard.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void CHSCANNER_init(void);
void CHSCANNER_deinit(void);
bool CHSCANNER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void CHSCANNER_update(void);
void CHSCANNER_render(void);

#endif /* end of include guard: CHANNELSCANNER_H */
