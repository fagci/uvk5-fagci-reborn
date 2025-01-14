#ifndef UI_SPECTRUM_H
#define UI_SPECTRUM_H

#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include <stdbool.h>
#include <stdint.h>

void SP_AddPoint(const Measurement *msm);
void SP_ResetHistory();
void SP_Init(Band *b);
void SP_Begin();
void SP_Render(const Band *p);
void SP_RenderRssi(uint16_t rssi, char *text, bool top);
void SP_RenderLine(uint16_t rssi);
void SP_RenderArrow(const Band *p, uint32_t f);
uint16_t SP_GetNoiseFloor();
uint8_t SP_GetNoiseMax();
uint16_t SP_GetRssiMax();

void SP_RenderGraph();
void SP_AddGraphPoint(const Measurement *msm);
void SP_Shift(int16_t n);
void SP_ShiftGraph(int16_t n);

bool SP_HasStats();
void SP_UpdateScanStats();
bool SP_IsSquelchOpen(const Measurement *msm);
extern uint8_t gNoiseOpenDiff;

extern uint8_t SPECTRUM_Y;
extern uint8_t SPECTRUM_H;

#endif /* end of include guard: UI_SPECTRUM_H */
