#ifndef MENU_H
#define MENU_H

#include <stdbool.h>
#include <stdint.h>

static const uint8_t MENU_Y = 8;

typedef struct MenuItem {
  const char *name;
  uint8_t type;
  uint8_t size;
  void (*getItemText)(uint16_t index, char *name);
} MenuItem;

typedef enum {
  MT_ITEMS,
  MT_RANGE,
  MT_INPUT,
  MT_RUN,
} MenuItemType;

void UI_ShowMenuItem(uint8_t line, const char *name, bool isCurrent);
void UI_ShowMenuSimple(const MenuItem *menu, uint16_t size, uint16_t currentIndex);
void UI_ShowMenu(void (*getItemText)(uint16_t index, char *name), uint16_t size,
                 uint16_t currentIndex);
void UI_ShowMenuEx(void (*showItem)(uint16_t i, uint16_t index, bool isCurrent),
                   uint16_t size, uint16_t currentIndex, uint16_t linesMax);

#endif /* end of include guard: MENU_H */
