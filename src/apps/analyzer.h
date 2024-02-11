#ifndef ANALYZER_H
#define ANALYZER_H

#include "../driver/keyboard.h"
#include "../radio.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

bool ANALYZER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void ANALYZER_init(void);
void ANALYZER_deinit(void);
void ANALYZER_update(void);
void ANALYZER_render(void);


#endif /* end of include guard: ANALYZER_H */
