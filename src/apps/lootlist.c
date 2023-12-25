#include "lootlist.h"
#include "../driver/st7565.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include <string.h>

#define ITEMS(value)                                                           \
  for (uint8_t i = 0; i < ARRAY_SIZE(value); ++i) {                            \
    strncpy(items[i], value[i], 15);                                           \
  }                                                                            \
  size = ARRAY_SIZE(value);                                                    \
  type = MT_ITEMS;

static uint8_t menuIndex = 0;

void LOOTLIST_render() {
  UI_ClearScreen();
  char items[LOOT_SIZE_MAX][16] = {0};
  for (uint8_t i = 0; i < LOOT_Size(); ++i) {
    Loot *item = LOOT_Item(i);
    uint32_t f = item->f;
    sprintf(items[i], "%u.%05u %us", f / 100000, f % 100000,
            item->duration / 1000);
  }
  UI_ShowItems(items, LOOT_Size(), menuIndex);
}

void LOOTLIST_init() { gRedrawScreen = true; }
void LOOTLIST_update() {}
bool LOOTLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  const Loot *item = LOOT_Item(menuIndex);
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
    gCurrentVFO->fRX = item->f;
    APPS_run(APP_STILL);
    return true;
  default:
    break;
  }
  return false;
}
