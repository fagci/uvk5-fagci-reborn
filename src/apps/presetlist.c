#include "presetlist.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include <string.h>

static uint8_t menuIndex = 0;

static void getPresetText(uint16_t i, char *name) {
  Preset *item = PRESETS_Item(i);
  uint32_t fs = item->band.bounds.start;
  uint32_t fe = item->band.bounds.end;
  if (item->band.name[0] > 32) {
    sprintf(name, "%s", item->band.name);
  } else {
    sprintf(name, "%u.%05u-%u.%05u", fs / 100000, fs % 100000, fe / 100000,
            fe % 100000);
  }
}

void PRESETLIST_render() {
  UI_ClearScreen();
  UI_ShowMenu(getPresetText, PRESETS_Size(), menuIndex);
}

void PRESETLIST_init() {
  gRedrawScreen = true;
  menuIndex = gSettings.activePreset;
}
void PRESETLIST_update() {}
bool PRESETLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  // const Preset *item = PRESETS_Item(menuIndex);
  const uint8_t MENU_SIZE = PRESETS_Size();
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
    PRESET_Select(menuIndex);
    APPS_run(APP_SPECTRUM);
    return true;
  case KEY_MENU:
    PRESET_Select(menuIndex);
    APPS_run(APP_PRESET_CFG);
    return true;
  default:
    break;
  }
  return false;
}
