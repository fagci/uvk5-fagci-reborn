#include "presetlist.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../helper/presetlist.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "apps.h"
#include "presetcfg.h"

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

void PRESETLIST_render(void) {
  UI_ClearScreen();
  UI_ShowMenu(getPresetText, PRESETS_Size(), menuIndex);
}

void PRESETLIST_init(void) {
  gRedrawScreen = true;
  menuIndex = gSettings.activePreset;
}

static void setMenuIndexAndRun(uint16_t v) {
  menuIndex = v - 1;
  RADIO_SelectPresetSave(menuIndex);
  APPS_exit();
}

bool PRESETLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  const uint8_t MENU_SIZE = PRESETS_Size();
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key >= KEY_0 && key <= KEY_9) {
      NUMNAV_Init(menuIndex + 1, 1, MENU_SIZE);
      gNumNavCallback = setMenuIndexAndRun;
    }
    if (gIsNumNavInput) {
      menuIndex = NUMNAV_Input(key) - 1;
      return true;
    }
  }
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
  case KEY_MENU:
    RADIO_SelectPresetSave(menuIndex);
    APPS_exit();
    return true;
  case KEY_F:
    presetCfgIndex = menuIndex;
    APPS_run(APP_PRESET_CFG);
    return true;
  default:
    break;
  }
  return false;
}
