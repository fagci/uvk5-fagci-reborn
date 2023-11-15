#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <stdint.h>

void UI_Battery(uint8_t Level);
void UI_RSSIBar(int16_t rssi, uint8_t line);
void UI_F(uint32_t f, uint8_t line);

#endif /* end of include guard: COMPONENTS_H */
