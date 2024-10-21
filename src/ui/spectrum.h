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
void SP_RenderRssi(uint16_t rssi, char *text, bool top, uint8_t sx, uint8_t sy,
                   uint8_t sh);
void SP_RenderLine(uint16_t rssi, uint8_t sx, uint8_t sy, uint8_t sh);
void SP_RenderArrow(Preset *p, uint32_t f, uint8_t sx, uint8_t sy, uint8_t sh);
uint16_t SP_GetNoiseFloor();
uint16_t SP_GetNoiseMax();
uint16_t SP_GetRssiMax();

#endif /* end of include guard: UI_SPECTRUM_H */
