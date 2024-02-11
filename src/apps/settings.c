#include "settings.h"
#include "../driver/backlight.h"
#include "../driver/st7565.h"
#include "../helper/battery.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../radio.h"
#include "../settings.h"
#include "../svc_scan.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include <string.h>

typedef enum {
  M_NONE,
  M_UPCONVERTER,
  M_MAIN_APP,
  M_SCAN_DELAY,
  M_SQL_OPEN_T,
  M_SQL_CLOSE_T,
  M_SQL_TO_OPEN,
  M_SQL_TO_CLOSE,
  M_BRIGHTNESS,
  M_BL_TIME,
  M_BL_SQL,
  M_FLT_BOUND,
  M_BEEP,
  M_BAT_CAL,
  M_BAT_TYPE,
  M_BAT_STYLE,
  M_RESET,
} Menu;

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static char Output[16];

const char *fltBound[] = {"240MHz", "280MHz"};

const uint16_t BAT_CAL_MIN = 1900;
const uint16_t BAT_CAL_MAX = 2155;

static const MenuItem menu[] = {
    {"Upconverter", M_UPCONVERTER, ARRAY_SIZE(upConverterFreqNames)},
    {"Main app", M_MAIN_APP, ARRAY_SIZE(appsAvailableToRun)},
    {"SQL open time", M_SQL_OPEN_T, 7},
    {"SQL close time", M_SQL_CLOSE_T, 3},
    {"SCAN single freq time", M_SCAN_DELAY, 255},
    {"SCAN listen time", M_SQL_TO_OPEN, ARRAY_SIZE(SCAN_TIMEOUT_NAMES)},
    {"SCAN after close time", M_SQL_TO_CLOSE, ARRAY_SIZE(SCAN_TIMEOUT_NAMES)},
    {"Brightness", M_BRIGHTNESS, 16},
    {"BL time", M_BL_TIME, ARRAY_SIZE(BL_TIME_VALUES)},
    {"BL SQL mode", M_BL_SQL, ARRAY_SIZE(BL_SQL_MODE_NAMES)},
    {"Filter bound", M_FLT_BOUND, 2},
    {"Beep", M_BEEP, 2},
    {"BAT calibration", M_BAT_CAL, 255},
    {"BAT type", M_BAT_TYPE, ARRAY_SIZE(BATTERY_TYPE_NAMES)},
    {"BAT style", M_BAT_STYLE, ARRAY_SIZE(BATTERY_STYLE_NAMES)},
    {"EEPROM reset", M_RESET, 2},
};

static void getSubmenuItemText(uint16_t index, char *name) {
  const MenuItem *item = &menu[menuIndex];
  uint16_t v =
      gBatteryVoltage * gSettings.batteryCalibration / (index + BAT_CAL_MIN);
  switch (item->type) {
  case M_UPCONVERTER:
    strncpy(name, upConverterFreqNames[index], 31);
    return;
  case M_MAIN_APP:
    strncpy(name, apps[appsAvailableToRun[index]].name, 31);
    return;
  case M_SCAN_DELAY:
    sprintf(name, "%ums", index);
    return;
  case M_SQL_OPEN_T:
    sprintf(name, "%ums", index * 5);
    return;
  case M_SQL_CLOSE_T:
    sprintf(name, "%ums", index * 5);
    return;
  case M_SQL_TO_OPEN:
    strncpy(name, SCAN_TIMEOUT_NAMES[index], 31);
    return;
  case M_SQL_TO_CLOSE:
    strncpy(name, SCAN_TIMEOUT_NAMES[index], 31);
    return;
  case M_BL_SQL:
    strncpy(name, BL_SQL_MODE_NAMES[index], 31);
    return;
  case M_FLT_BOUND:
    strncpy(name, fltBound[index], 31);
    return;
  case M_BRIGHTNESS:
    sprintf(name, "%u", index);
    return;
  case M_BL_TIME:
    strncpy(name, BL_TIME_NAMES[index], 31);
    return;
  case M_BEEP:
    strncpy(name, onOff[index], 31);
    return;
  case M_BAT_CAL:
    sprintf(name, "%u.%02u (%u)", v / 100, v % 100, index + BAT_CAL_MIN);
    return;
  case M_BAT_TYPE:
    strncpy(name, BATTERY_TYPE_NAMES[index], 31);
    return;
  case M_BAT_STYLE:
    strncpy(name, BATTERY_STYLE_NAMES[index], 31);
    return;
  case M_RESET:
    strncpy(name, yesNo[index], 31);
    return;
  default:
    break;
  }
}

static void accept(void) {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_UPCONVERTER: {
    uint32_t f = GetScreenF(gCurrentVFO->fRX);
    gSettings.upconverter = subMenuIndex;
    RADIO_TuneToSave(GetTuneF(f));
    SETTINGS_Save();
  }; break;
  case M_MAIN_APP:
    gSettings.mainApp = appsAvailableToRun[subMenuIndex];
    SETTINGS_Save();
    break;
  case M_SCAN_DELAY:
    gSettings.scanTimeout = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_SQL_OPEN_T:
    gSettings.sqlOpenTime = subMenuIndex;
    RADIO_SetupByCurrentVFO();
    SETTINGS_Save();
    break;
  case M_SQL_CLOSE_T:
    gSettings.sqlCloseTime = subMenuIndex;
    RADIO_SetupByCurrentVFO();
    SETTINGS_Save();
    break;
  case M_SQL_TO_OPEN:
    gSettings.sqOpenedTimeout = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_SQL_TO_CLOSE:
    gSettings.sqClosedTimeout = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_BL_SQL:
    gSettings.backlightOnSquelch = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_FLT_BOUND:
    gSettings.bound_240_280 = subMenuIndex;
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
  case M_BEEP:
    gSettings.beep = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_BAT_CAL:
    gSettings.batteryCalibration = subMenuIndex + BAT_CAL_MIN;
    SETTINGS_Save();
    break;
  case M_BAT_TYPE:
    gSettings.batteryType = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_BAT_STYLE:
    gSettings.batteryStyle = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_RESET:
    if (subMenuIndex) {
      APPS_run(APP_RESET);
    }
    break;
  default:
    break;
  }
}

static const char *getValue(Menu type) {
  switch (type) {
  case M_BRIGHTNESS:
    sprintf(Output, "%u", gSettings.brightness);
    return Output;
  case M_BAT_CAL:
    sprintf(Output, "%u", gSettings.batteryCalibration);
    return Output;
  case M_SCAN_DELAY:
    sprintf(Output, "%ums", gSettings.scanTimeout);
    return Output;
  case M_SQL_OPEN_T:
    sprintf(Output, "%ums", gSettings.sqlOpenTime * 5);
    return Output;
  case M_SQL_CLOSE_T:
    sprintf(Output, "%ums", gSettings.sqlCloseTime * 5);
    return Output;
  case M_BAT_TYPE:
    return BATTERY_TYPE_NAMES[gSettings.batteryType];
  case M_BAT_STYLE:
    return BATTERY_STYLE_NAMES[gSettings.batteryStyle];
  case M_MAIN_APP:
    return apps[gSettings.mainApp].name;
  case M_SQL_TO_OPEN:
    return SCAN_TIMEOUT_NAMES[gSettings.sqOpenedTimeout];
  case M_SQL_TO_CLOSE:
    return SCAN_TIMEOUT_NAMES[gSettings.sqClosedTimeout];
  case M_BL_TIME:
    return BL_TIME_NAMES[gSettings.backlight];
  case M_BL_SQL:
    return BL_SQL_MODE_NAMES[gSettings.backlightOnSquelch];
  case M_FLT_BOUND:
    sprintf(Output, "%uMHz", SETTINGS_GetFilterBound() / 100000);
    return Output;
  case M_UPCONVERTER:
    return upConverterFreqNames[gSettings.upconverter];
  case M_BEEP:
    return onOff[gSettings.beep];
  default:
    break;
  }
  return "";
}

static void onSubChange(void) {
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

static void setInitialSubmenuIndex(void) {
  const MenuItem *item = &menu[menuIndex];
  uint8_t i = 0;
  switch (item->type) {
  case M_BRIGHTNESS:
    subMenuIndex = gSettings.brightness;
    break;
  case M_BL_TIME:
    subMenuIndex = gSettings.backlight;
    break;
  case M_SCAN_DELAY:
    subMenuIndex = gSettings.scanTimeout;
    break;
  case M_SQL_OPEN_T:
    subMenuIndex = gSettings.sqlOpenTime;
    break;
  case M_SQL_CLOSE_T:
    subMenuIndex = gSettings.sqlCloseTime;
    break;
  case M_SQL_TO_OPEN:
    subMenuIndex = gSettings.sqOpenedTimeout;
    break;
  case M_SQL_TO_CLOSE:
    subMenuIndex = gSettings.sqClosedTimeout;
    break;
  case M_BL_SQL:
    subMenuIndex = gSettings.backlightOnSquelch;
    break;
  case M_FLT_BOUND:
    subMenuIndex = gSettings.bound_240_280;
    break;
  case M_UPCONVERTER:
    subMenuIndex = gSettings.upconverter;
    break;
  case M_BEEP:
    subMenuIndex = gSettings.beep;
    break;
  case M_BAT_CAL:
    subMenuIndex = gSettings.batteryCalibration - BAT_CAL_MIN;
    break;
  case M_BAT_TYPE:
    subMenuIndex = gSettings.batteryType;
    break;
  case M_BAT_STYLE:
    subMenuIndex = gSettings.batteryStyle;
    break;
  case M_MAIN_APP:
    for (i = 0; i < ARRAY_SIZE(appsAvailableToRun); ++i) {
      if (appsAvailableToRun[i] == gSettings.mainApp) {
        subMenuIndex = i;
        break;
      }
    }
    break;
  default:
    subMenuIndex = 0;
    break;
  }
}

void SETTINGS_init(void) { gRedrawScreen = true; }

void SETTINGS_update(void) {}

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
void SETTINGS_render(void) {
  UI_ClearScreen();
  const MenuItem *item = &menu[menuIndex];
  if (isSubMenu) {
    UI_ShowMenu(getSubmenuItemText, item->size, subMenuIndex);
    STATUSLINE_SetText(item->name);
  } else {
    UI_ShowMenuSimple(menu, ARRAY_SIZE(menu), menuIndex);
    PrintMediumEx(LCD_XCENTER, LCD_HEIGHT - 4, POS_C, C_FILL,
                  getValue(item->type));
  }
}
