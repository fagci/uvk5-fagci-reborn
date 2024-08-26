#include "presetlist.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../helper/presetlist.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "apps.h"

static uint8_t menuIndex = 0;

static void getPresetText(uint16_t i, char *name) {
  sprintf(name, "%s", PRESETS_Item(i)->band.name);
}

void PRESETLIST_render(void) {
  UI_ClearScreen();
  UI_ShowMenu(getPresetText, PRESETS_Size(), menuIndex);
}

void PRESETLIST_init(void) { menuIndex = gSettings.activePreset; }

static void setMenuIndexAndRun(uint16_t v) {
  menuIndex = v - 1;
  RADIO_SelectPresetSave(menuIndex);
  APPS_exit();
}

bool PRESETLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  const uint8_t MENU_SIZE = PRESETS_Size();
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key <= KEY_9) {
      NUMNAV_Init(menuIndex + 1, 1, MENU_SIZE);
      gNumNavCallback = setMenuIndexAndRun;
    }
    if (gIsNumNavInput) {
      menuIndex = NUMNAV_Input(key) - 1;
      return true;
    }
  }
  // Simple keypress
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_EXIT:
      APPS_exit();
      return true;
    case KEY_MENU:
      RADIO_SelectPresetSave(menuIndex);
      APPS_exit();
      return true;
    case KEY_F:
      PRESET_Select(menuIndex);
      APPS_run(APP_PRESET_CFG);
      return true;
    default:
      break;
    }
  }
  switch (key) {
  case KEY_UP:
    IncDec8(&menuIndex, 0, MENU_SIZE, -1);
    return true;
  case KEY_DOWN:
    IncDec8(&menuIndex, 0, MENU_SIZE, 1);
    return true;
  default:
    break;
  }
  return false;
}
