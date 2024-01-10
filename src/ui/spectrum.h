#ifndef UI_SPECTRUM_H
#define UI_SPECTRUM_H

#include "../helper/lootlist.h"
#include "../helper/presetlist.h"
#include <stdbool.h>
#include <stdint.h>

void SP_AddPoint(Loot *msm);
void SP_ResetHistory();
void SP_Init(uint16_t steps, uint8_t width);
void SP_Begin();
void SP_Next();
void SP_Render(Preset *p, uint8_t x, uint8_t y, uint8_t h);

#endif /* end of include guard: UI_SPECTRUM_H */
