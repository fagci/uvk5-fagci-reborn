#include "presetlist.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../helper/presetlist.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "apps.h"
#include "savech.h"

static uint8_t menuIndex = 0;

static void getPresetItem(uint16_t i, uint16_t index, bool isCurrent) {
  Preset *p = PRESETS_Item(index);
  const uint8_t y = MENU_Y + i * MENU_ITEM_H;
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  PrintMediumEx(8, y + 8, POS_L, C_INVERT, "%s", p->band.name);
  char scanlistsStr[9] = "";
  for (uint8_t n = 0; n < 8; ++n) {
    scanlistsStr[n] = p->memoryBanks & (1 << n) ? '1' + n : '-';
  }
  PrintSmallEx(LCD_WIDTH - 5, y + 8, POS_R, C_INVERT, "%s", scanlistsStr);
}

static void toggleScanlist(uint8_t n) {
  Preset *p = PRESETS_Item(menuIndex);
  p->memoryBanks ^= 1 << n;
  PRESETS_SavePreset(menuIndex, p);
}

void PRESETLIST_render(void) {
  UI_ClearScreen();
  UI_ShowMenuEx(getPresetItem, PRESETS_Size(), menuIndex,
                MENU_LINES_TO_SHOW + 1);
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
    if (!gIsNumNavInput && key == KEY_STAR) {
      NUMNAV_Init(menuIndex + 1, 1, MENU_SIZE);
      gNumNavCallback = setMenuIndexAndRun;
      return true;
    }
    if (gIsNumNavInput) {
      menuIndex = NUMNAV_Input(key) - 1;
      return true;
    }
  }
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    if (SAVECH_SelectScanlist(key)) {
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
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
      toggleScanlist(key - KEY_1);
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
