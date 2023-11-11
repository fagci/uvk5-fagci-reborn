#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include <stdbool.h>
#include <stdint.h>

void BACKLIGHT_Toggle(bool on);
void BACKLIGHT_On();
void BACKLIGHT_Update();
void BACKLIGHT_SetDuration(uint8_t durationSec);

#endif /* end of include guard: BACKLIGHT_H */
