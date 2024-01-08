#ifndef SPECTRUMCHANNEL_H
#define SPECTRUMCHANNEL_H

#include "../driver/keyboard.h"
#include "../radio.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

bool SPECTRUMCH_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void SPECTRUMCH_init(void);
void SPECTRUMCH_update(void);
void SPECTRUMCH_render(void);

#endif /* end of include guard: SPECTRUMCHANNEL_H */
