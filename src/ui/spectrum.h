#ifndef UI_SPECTRUM_H
#define UI_SPECTRUM_H

#include "../helper/lootlist.h"
#include "../helper/presetlist.h"
#include <stdbool.h>
#include <stdint.h>

void SP_AddPoint(Loot *msm);
void SP_ResetHistory();
void SP_Init(uint16_t steps);
void SP_Begin();
void SP_Next();
void SP_Render(Preset *p);
void SP_RenderRssi(uint16_t rssi, char *text, bool top);
void SP_RenderLine(uint16_t rssi);
void SP_RenderArrow(Preset *p, uint32_t f);
uint16_t SP_GetNoiseFloor();
uint8_t SP_GetNoiseMax();
uint16_t SP_GetRssiMax();

void SP_RenderGraph(uint8_t sx, uint8_t sy, uint8_t sh);
void SP_AddGraphPoint(Loot *msm);
void SP_Shift(int16_t n);

extern const uint8_t SPECTRUM_Y;
extern const uint8_t SPECTRUM_H;

#endif /* end of include guard: UI_SPECTRUM_H */
