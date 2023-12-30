#include "settings.h"
#include "../driver/backlight.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../radio.h"
#include "../settings.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"
#include <string.h>

typedef enum {
  M_NONE,
  M_UPCONVERTER,
  M_MAIN_APP,
  M_BRIGHTNESS,
  M_BL_TIME,
  M_RESET,
} Menu;

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static const MenuItem menu[] = {
    {"Upconverter", M_UPCONVERTER, ARRAY_SIZE(upConverterFreqNames)},
    {"Main app", M_MAIN_APP, ARRAY_SIZE(appsAvailableToRun)},
    {"Brightness", M_BRIGHTNESS, 16},
    {"BL time", M_BL_TIME, ARRAY_SIZE(BL_TIME_VALUES)},
    {"EEPROM reset", M_RESET},
};

static void accept() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_UPCONVERTER: {
    uint32_t f = GetScreenF(gCurrentVFO->fRX);
    gSettings.upconverter = subMenuIndex;
    RADIO_TuneTo(GetTuneF(f));
    SETTINGS_Save();
  }; break;
  case M_MAIN_APP:
    gSettings.mainApp = appsAvailableToRun[subMenuIndex];
    SETTINGS_Save();
    break;
  case M_BRIGHTNESS:
    gSettings.brightness = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_BL_TIME:
    gSettings.backlight = subMenuIndex;
    SETTINGS_Save();
    break;
  default:
    break;
  }
}

char Output[16];

static const char *getValue(Menu type) {
  switch (type) {
  case M_BRIGHTNESS:
    sprintf(Output, "%u", gSettings.brightness);
    return Output;
  case M_MAIN_APP:
    return apps[gSettings.mainApp].name;
  case M_BL_TIME:
    return BL_TIME_NAMES[gSettings.backlight];
  case M_UPCONVERTER:
    return upConverterFreqNames[gSettings.upconverter];
  default:
    break;
  }
  return "";
}

static void getMenuItemText(uint16_t index, char *name) {
  strncpy(name, menu[index].name, 31);
}

static void getBrightnessLevelText(uint16_t index, char *name) {
  sprintf(name, "%u", index);
}

static void getUCTypeText(uint16_t index, char *name) {
  strncpy(name, upConverterFreqNames[index], 31);
}

static void getBacklightTimeText(uint16_t index, char *name) {
  strncpy(name, BL_TIME_NAMES[index], 31);
}

static void getMainAppText(uint16_t index, char *name) {
  strncpy(name, apps[appsAvailableToRun[index]].name, 31);
}

static void showSubmenu(Menu menuType) {
  const MenuItem *item = &menu[menuIndex];
  switch (menuType) {
  case M_UPCONVERTER:
    UI_ShowMenu(getUCTypeText, ARRAY_SIZE(upConverterFreqNames), subMenuIndex);
    break;
  case M_MAIN_APP:
    UI_ShowMenu(getMainAppText, ARRAY_SIZE(apps), subMenuIndex);
    break;
  case M_BL_TIME:
    UI_ShowMenu(getBacklightTimeText, ARRAY_SIZE(BL_TIME_NAMES), subMenuIndex);
    break;
  case M_BRIGHTNESS:
    UI_ShowMenu(getBrightnessLevelText, item->size, subMenuIndex);
    break;
  default:
    break;
  }
}

static void onSubChange() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_BRIGHTNESS:
    BACKLIGHT_SetBrightness(subMenuIndex);
    break;
  case M_BL_TIME:
    BACKLIGHT_SetDuration(BL_TIME_VALUES[subMenuIndex]);
    break;
  default:
    break;
  }
}

static void setInitialSubmenuIndex() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_BRIGHTNESS:
    subMenuIndex = gSettings.brightness;
    break;
  case M_BL_TIME:
    subMenuIndex = gSettings.backlight;
    break;
  case M_UPCONVERTER:
    subMenuIndex = gSettings.upconverter;
    break;
  default:
    subMenuIndex = 0;
    break;
  }
}

void SETTINGS_init() { gRedrawScreen = true; }
void SETTINGS_update() {}
bool SETTINGS_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  const MenuItem *item = &menu[menuIndex];
  const uint8_t MENU_SIZE = ARRAY_SIZE(menu);
  const uint8_t SUBMENU_SIZE = item->size;
  switch (key) {
  case KEY_UP:
    if (isSubMenu) {
      IncDec8(&subMenuIndex, 0, SUBMENU_SIZE, -1);
      onSubChange();
    } else {
      IncDec8(&menuIndex, 0, MENU_SIZE, -1);
    }
    return true;
  case KEY_DOWN:
    if (isSubMenu) {
      IncDec8(&subMenuIndex, 0, SUBMENU_SIZE, 1);
      onSubChange();
    } else {
      IncDec8(&menuIndex, 0, MENU_SIZE, 1);
    }
    return true;
  case KEY_MENU:
    // RUN APPS HERE
    switch (item->type) {
    case M_RESET:
      APPS_run(APP_RESET);
      return true;
    default:
      break;
    }
    if (isSubMenu) {
      accept();
      isSubMenu = false;
    } else if (!bKeyHeld) {
      isSubMenu = true;
      setInitialSubmenuIndex();
    }
    return true;
  case KEY_EXIT:
    if (isSubMenu) {
      isSubMenu = false;
    } else {
      APPS_exit();
    }
    return true;
  default:
    break;
  }
  return false;
}
void SETTINGS_render() {
  UI_ClearScreen();
  UI_ClearStatus();
  const MenuItem *item = &menu[menuIndex];
  if (isSubMenu) {
    showSubmenu(item->type);
    STATUSLINE_SetText(item->name);
  } else {
    UI_ShowMenu(getMenuItemText, ARRAY_SIZE(menu), menuIndex);
    PrintMediumEx(LCD_WIDTH / 2, 6 * 8 + 12, POS_C, C_FILL,
                  getValue(item->type));
  }
}
