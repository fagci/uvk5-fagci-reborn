#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "../radio.h"
#include <stdbool.h>
#include <stdint.h>

#define BATTERY_W 13

void UI_Battery(uint8_t Level);
void UI_RSSIBar(uint16_t rssi, uint8_t snr, uint32_t f, uint8_t y);
void UI_DrawScrollBar(const uint16_t size, const uint16_t currentIndex,
                      const uint8_t linesCount);
void UI_DrawTicks(uint8_t y, const Band *band);
void UI_DrawSpectrumElements(const uint8_t sy, uint8_t msmDelay, int16_t sq,
                             Band *currentBand);
void UI_ShowWait();

#endif /* end of include guard: COMPONENTS_H */
