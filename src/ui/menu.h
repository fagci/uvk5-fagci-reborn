#ifndef MENU_H
#define MENU_H

#include <stdbool.h>
#include <stdint.h>

static const uint8_t MENU_Y = 8;
static const uint8_t MENU_ITEM_H = 11;
static const uint8_t MENU_LINES_TO_SHOW = 4;
extern const char *onOff[];
extern const char *yesNo[];

typedef enum {
  M_RADIO,
  M_START,
  M_END,
  M_NAME,
  M_STEP,
  M_MODULATION,
  M_BW,
  M_SQ,
  M_SQ_TYPE,
  M_GAIN,
  M_TX,
  M_F_RX,
  M_F_TX,
  M_RX_CODE_TYPE,
  M_RX_CODE,
  M_TX_CODE_TYPE,
  M_TX_CODE,
  M_TX_OFFSET,
  M_TX_OFFSET_DIR,
  M_F_TXP,
  M_SAVE,
} PresetCfgMenu;

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

void UI_ShowMenuItem(uint8_t line, const char *name, bool isCurrent);
void UI_ShowMenuSimple(const MenuItem *menu, uint16_t size,
                       uint16_t currentIndex);
void UI_ShowMenu(void (*getItemText)(uint16_t index, char *name), uint16_t size,
                 uint16_t currentIndex);
void UI_ShowMenuEx(void (*showItem)(uint16_t i, uint16_t index, bool isCurrent),
                   uint16_t size, uint16_t currentIndex, uint16_t linesMax);

void GetMenuItemValue(PresetCfgMenu type, char *Output, int8_t presetIndex);
void AcceptRadioConfig(const MenuItem *item, uint8_t subMenuIndex, int8_t presetIndex);

#endif /* end of include guard: MENU_H */
