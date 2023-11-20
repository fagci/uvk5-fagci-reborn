#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include <stdbool.h>
#include <stdint.h>

void BACKLIGHT_Toggle(bool on);
void BACKLIGHT_On();
void BACKLIGHT_Update();
void BACKLIGHT_SetDuration(uint8_t durationSec);
void BACKLIGHT_Init();
extern uint8_t duration;
extern uint8_t countdown;
extern bool state;

#endif /* end of include guard: BACKLIGHT_H */
