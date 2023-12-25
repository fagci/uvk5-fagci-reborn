#include "lootlist.h"
#include "../driver/st7565.h"
#include "../driver/uart.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"
#include <string.h>

static uint8_t menuIndex = 0;
static const uint8_t MENU_ITEM_H = 15;
static enum {
  SORT_LOT,
  SORT_DUR,
  SORT_BL,
  SORT_F,
} sortType;
static char *sortNames[] = {
    "last open time",
    "duration",
    "blacklist",
    "frequency",
};

static void getLootItem(uint16_t i, bool isCurrent) {
  Loot *item = LOOT_Item(i);
  uint32_t f = item->f;
  const uint8_t y = 8 + i * MENU_ITEM_H;
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  PrintMediumEx(6, y + 7, POS_L, C_INVERT, "%u.%05u", f / 100000, f % 100000);
  PrintSmallEx(LCD_WIDTH - 6, y + 7, POS_R, C_INVERT, "%us",
               item->duration / 1000);
  PrintSmallEx(6, y + 7 + 6, POS_L, C_INVERT, "CT:%u CD:%u R:%u N:%u", item->ct,
               item->cd, item->rssi, item->noise);
  if (item->blacklist) {
    DrawHLine(2, y + 5, LCD_WIDTH - 4, C_INVERT);
  }
}

void LOOTLIST_render() {
  UI_ClearScreen();
  UI_ShowMenuEx(getLootItem, LOOT_Size(), menuIndex, 4);
}

void LOOTLIST_init() { gRedrawScreen = true; }
void LOOTLIST_update() {}
bool LOOTLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  Loot *item = LOOT_Item(menuIndex);
  const uint8_t MENU_SIZE = LOOT_Size();
  switch (key) {
  case KEY_UP:
    IncDec8(&menuIndex, 0, MENU_SIZE, -1);
    return true;
  case KEY_DOWN:
    IncDec8(&menuIndex, 0, MENU_SIZE, 1);
    return true;
  case KEY_EXIT:
    APPS_exit();
    return true;
  case KEY_PTT:
    RADIO_TuneToSave(item->f);
    APPS_run(APP_STILL);
    return true;
  case KEY_1:
    LOOT_Sort(LOOT_SortByLastOpenTime);
    sortType = SORT_LOT;
    STATUSLINE_SetText("By %s", sortNames[sortType]);
    return true;
  case KEY_2:
    LOOT_Sort(LOOT_SortByDuration);
    sortType = SORT_DUR;
    STATUSLINE_SetText("By %s", sortNames[sortType]);
    return true;
  case KEY_3:
    LOOT_Sort(LOOT_SortByBlacklist);
    sortType = SORT_BL;
    STATUSLINE_SetText("By %s", sortNames[sortType]);
    return true;
  case KEY_4:
    LOOT_Sort(LOOT_SortByF);
    sortType = SORT_F;
    STATUSLINE_SetText("By %s", sortNames[sortType]);
    return true;
  case KEY_SIDE1:
    item->blacklist = !item->blacklist;
    return true;
  default:
    break;
  }
  return false;
}
