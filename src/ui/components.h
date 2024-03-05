#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "../radio.h"
#include <stdint.h>

#define BATTERY_W 13

void UI_Battery(uint8_t Level);
void UI_RSSIBar(uint16_t rssi, uint32_t f, uint8_t y);
void UI_FSmall(uint32_t f);
void UI_FSmallest(uint32_t f, uint8_t x, uint8_t y);
void UI_DrawScrollBar(const uint16_t size, const uint16_t currentIndex,
                      const uint8_t linesCount);
void UI_DrawTicks(uint8_t x1, uint8_t x2, uint8_t y, FRange *range);

#endif /* end of include guard: COMPONENTS_H */
