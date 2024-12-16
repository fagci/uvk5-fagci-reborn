#ifndef UI_SPECTRUM_H
#define UI_SPECTRUM_H

#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include <stdbool.h>
#include <stdint.h>

void SP_AddPoint(const Loot *msm);
void SP_ResetHistory();
void SP_Init(Band *b);
void SP_Begin();
void SP_Next();
void SP_Render(const Band *p);
void SP_RenderRssi(uint16_t rssi, char *text, bool top);
void SP_RenderLine(uint16_t rssi);
void SP_RenderArrow(const Band *p, uint32_t f);
uint16_t SP_GetNoiseFloor();
uint8_t SP_GetNoiseMax();
uint16_t SP_GetRssiMax();

void SP_RenderGraph();
void SP_AddGraphPoint(const Loot *msm);
void SP_Shift(int16_t n);

extern const uint8_t SPECTRUM_Y;
extern const uint8_t SPECTRUM_H;

#endif /* end of include guard: UI_SPECTRUM_H */
