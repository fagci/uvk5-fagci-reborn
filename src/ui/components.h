#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct MenuItem {
  const char *name;
  uint8_t type;
  uint8_t size;
} MenuItem;

typedef enum {
  MT_ITEMS,
  MT_RANGE,
  MT_INPUT,
  MT_RUN,
} MenuItemType;

void UI_Battery(uint8_t Level);
void UI_RSSIBar(int16_t rssi, uint32_t f, uint8_t line);
void UI_F(uint32_t f, uint8_t line);
void UI_FSmall(uint32_t f);
void UI_DrawScrollBar(const uint16_t size, const uint16_t currentIndex,
                      const uint8_t linesCount);
void UI_ShowMenuItem(uint8_t line, const char *name, bool isCurrent);
void UI_ShowMenu(const MenuItem *items, uint8_t size, uint8_t currentIndex);
void UI_ShowItems(char (*items)[16], uint16_t size, uint16_t currentIndex);

#endif /* end of include guard: COMPONENTS_H */
