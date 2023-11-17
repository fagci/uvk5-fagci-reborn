#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <stdint.h>

typedef struct MenuItem {
  const char *name;
  uint8_t type;
  uint8_t size;
} MenuItem;

void UI_Battery(uint8_t Level);
void UI_RSSIBar(int16_t rssi, uint8_t line);
void UI_F(uint32_t f, uint8_t line);
void UI_FSmall(uint32_t f);
void UI_DrawScrollBar(const uint8_t size, const uint8_t currentIndex,
                      const uint8_t linesCount);
void UI_ShowMenu(const MenuItem *items, uint8_t size, uint8_t currentIndex);
void UI_ShowItems(const char **items, uint8_t size, uint8_t currentIndex);

#endif /* end of include guard: COMPONENTS_H */
