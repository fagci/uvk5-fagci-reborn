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
#include "apps.h"
#include <string.h>

static uint8_t menuIndex = 0;

static void getLootItemText(uint16_t i, char *name) {
  Loot *item = LOOT_Item(i);
  uint32_t f = item->f;
  sprintf(name, "%u.%05u", f / 100000, f % 100000);
}

void LOOTLIST_render() {
  UI_ClearScreen();
  UI_ShowMenu(getLootItemText, LOOT_Size(), menuIndex);
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
    RADIO_TuneToSave(item->f);
    APPS_run(APP_STILL);
    return true;
  default:
    break;
  }
  return false;
}
