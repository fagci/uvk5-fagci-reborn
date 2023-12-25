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

void LOOTLIST_render() {
  UI_ClearScreen();
  char items[LOOT_SIZE_MAX][16] = {0};
  for (uint8_t i = 0; i < LOOT_Size(); ++i) {
    Loot *item = LOOT_Item(i);
    uint32_t f = item->f;
    snprintf(items[i], 15, "%u.%05u", f / 100000, f % 100000);
  }
  UI_ShowItems(items, LOOT_Size(), menuIndex);
}

void LOOTLIST_init() {
  gRedrawScreen = true;
  UART_logf(1, "[LOOTLIST] (0) %u presets", PRESETS_Size());
}
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
    UART_logf(1, "[LOOTLIST] (1) %u presets", PRESETS_Size());
    RADIO_TuneToSave(item->f);
    UART_logf(1, "[LOOTLIST] (2) %u presets", PRESETS_Size());
    APPS_run(APP_STILL);
    return true;
  default:
    break;
  }
  return false;
}
