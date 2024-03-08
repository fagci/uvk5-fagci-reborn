#include "settings.h"
#include "../driver/backlight.h"
#include "../driver/st7565.h"
#include "../helper/battery.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../misc.h"
#include "../radio.h"
#include "../settings.h"
#include "../svc_scan.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "textinput.h"
#include <string.h>

const uint8_t EEPROM_CHECKBYTE = 0b10101;

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
  M_CONTRAST,
  M_BL_TIME,
  M_BL_SQL,
  M_FLT_BOUND,
  M_BEEP,
  M_BAT_CAL,
  M_BAT_TYPE,
  M_BAT_STYLE,
  M_EEPROM_TYPE,
  M_NICKNAME,
  M_SKIP_GARBAGE_FREQS,
  M_RESET,
} Menu;

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static char Output[16];


const uint16_t BAT_CAL_MIN = 1900;
const uint16_t BAT_CAL_MAX = 2155;

static MenuItem menu[] = {
    {"Upconverter", M_UPCONVERTER, ARRAY_SIZE(UPCONVERTER_NAMES)},
    {"Main app", M_MAIN_APP, 0}, // will be ok at render
    {"SQL open time", M_SQL_OPEN_T, 7},
    {"SQL close time", M_SQL_CLOSE_T, 3},
    {"SCAN single freq time", M_SCAN_DELAY, 255},
    {"SCAN listen time", M_SQL_TO_OPEN, ARRAY_SIZE(SCAN_TIMEOUT_NAMES)},
    {"SCAN after close time", M_SQL_TO_CLOSE, ARRAY_SIZE(SCAN_TIMEOUT_NAMES)},
    {"Brightness", M_BRIGHTNESS, 16},
    {"Contrast", M_CONTRAST, 16},
    {"BL time", M_BL_TIME, ARRAY_SIZE(BL_TIME_VALUES)},
    {"BL SQL mode", M_BL_SQL, ARRAY_SIZE(BL_SQL_MODE_NAMES)},
    {"Filter bound", M_FLT_BOUND, 2},
    {"Beep", M_BEEP, 2},
    {"BAT calibration", M_BAT_CAL, 255},
    {"BAT type", M_BAT_TYPE, ARRAY_SIZE(BATTERY_TYPE_NAMES)},
    {"BAT style", M_BAT_STYLE, ARRAY_SIZE(BATTERY_STYLE_NAMES)},
    {"EEPROM type", M_EEPROM_TYPE, ARRAY_SIZE(EEPROM_TYPE_NAMES)},
    {"Nickname", M_NICKNAME, 0},
    {"Skip garbage freqs", M_SKIP_GARBAGE_FREQS, 2},
    {"EEPROM reset", M_RESET, 2},
};

static const uint8_t MENU_SIZE = ARRAY_SIZE(menu);

static void getSubmenuItemText(uint16_t index, char *name) {
  const MenuItem *item = &menu[menuIndex];
  uint16_t v =
      gBatteryVoltage * gSettings.batteryCalibration / (index + BAT_CAL_MIN);
  switch (item->type) {
  case M_UPCONVERTER:
    strncpy(name, UPCONVERTER_NAMES[index], 31);
    return;
  case M_MAIN_APP:
    strncpy(name, appsAvailableToRun[index]->name, 31);
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
    strncpy(name, FILTER_BOUND_NAMES[index], 31);
    return;
  case M_BRIGHTNESS:
    sprintf(name, "%u", index);
    return;
  case M_CONTRAST:
    sprintf(name, "%d", index - 8);
    return;
  case M_BL_TIME:
    strncpy(name, BL_TIME_NAMES[index], 31);
    return;
  case M_BEEP:
    strncpy(name, ON_OFF_NAMES[index], 31);
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
  case M_EEPROM_TYPE:
    strncpy(name, EEPROM_TYPE_NAMES[index], 31);
    return;
  case M_RESET:
    strncpy(name, YES_NO_NAMES[index], 31);
    return;
  case M_SKIP_GARBAGE_FREQS:
    strncpy(name, YES_NO_NAMES[index], 31);
    return;
  case M_NICKNAME:
    strncpy(name, gSettings.nickName, 31);
    return;
  default:
    break;
  }
}

static void setNickname() {
  strncpy(gSettings.nickName, gTextinputText, gTextInputSize);
  SETTINGS_Save();
}

static void accept() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_UPCONVERTER: {
    uint32_t f = GetScreenF(radio->f);
    gSettings.upconverter = subMenuIndex;
    RADIO_TuneToSave(GetTuneF(f));
    SETTINGS_Save();
  }; break;
  case M_MAIN_APP:
    gSettings.mainApp = appsAvailableToRun[subMenuIndex]->id;
    SETTINGS_Save();
    break;
  case M_SCAN_DELAY:
    radio->scan.timeout = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_SQL_OPEN_T:
    radio->sq.openTime = subMenuIndex;
    RADIO_SetupByCurrentCH();
    SETTINGS_Save();
    break;
  case M_SQL_CLOSE_T:
    radio->sq.closeTime = subMenuIndex;
    RADIO_SetupByCurrentCH();
    SETTINGS_Save();
    break;
  case M_SQL_TO_OPEN:
    radio->scan.openedTimeout = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_SQL_TO_CLOSE:
    radio->scan.closedTimeout = subMenuIndex;
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
  case M_CONTRAST:
    gSettings.contrast = subMenuIndex;
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
  case M_EEPROM_TYPE:
    gSettings.eepromType = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_SKIP_GARBAGE_FREQS:
    gSettings.skipGarbageFrequencies = subMenuIndex;
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
  case M_CONTRAST:
    sprintf(Output, "%d", gSettings.contrast - 8);
    return Output;
  case M_BAT_CAL:
    sprintf(Output, "%u", gSettings.batteryCalibration);
    return Output;
  case M_SCAN_DELAY:
    sprintf(Output, "%ums", radio->scan.timeout);
    return Output;
  case M_SQL_OPEN_T:
    sprintf(Output, "%ums", radio->sq.openTime * 5);
    return Output;
  case M_SQL_CLOSE_T:
    sprintf(Output, "%ums", radio->sq.closeTime * 5);
    return Output;
  case M_BAT_TYPE:
    return BATTERY_TYPE_NAMES[gSettings.batteryType];
  case M_BAT_STYLE:
    return BATTERY_STYLE_NAMES[gSettings.batteryStyle];
  case M_EEPROM_TYPE:
    return EEPROM_TYPE_NAMES[gSettings.eepromType];
  case M_MAIN_APP:
    return apps[gSettings.mainApp]->name;
  case M_SQL_TO_OPEN:
    return SCAN_TIMEOUT_NAMES[radio->scan.openedTimeout];
  case M_SQL_TO_CLOSE:
    return SCAN_TIMEOUT_NAMES[radio->scan.closedTimeout];
  case M_BL_TIME:
    return BL_TIME_NAMES[gSettings.backlight];
  case M_BL_SQL:
    return BL_SQL_MODE_NAMES[gSettings.backlightOnSquelch];
  case M_FLT_BOUND:
    sprintf(Output, "%uMHz", SETTINGS_GetFilterBound() / 100000);
    return Output;
  case M_UPCONVERTER:
    return UPCONVERTER_NAMES[gSettings.upconverter];
  case M_BEEP:
    return ON_OFF_NAMES[gSettings.beep];
  case M_SKIP_GARBAGE_FREQS:
    return YES_NO_NAMES[gSettings.skipGarbageFrequencies];
  case M_NICKNAME:
    return gSettings.nickName;
  default:
    break;
  }
  return "";
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
  case M_CONTRAST:
    gSettings.contrast = subMenuIndex;
    break;
  default:
    break;
  }
}

static void setInitialSubmenuIndex() {
  const MenuItem *item = &menu[menuIndex];
  uint8_t i = 0;
  switch (item->type) {
  case M_BRIGHTNESS:
    subMenuIndex = gSettings.brightness;
    break;
  case M_CONTRAST:
    subMenuIndex = gSettings.contrast;
    break;
  case M_BL_TIME:
    subMenuIndex = gSettings.backlight;
    break;
  case M_SCAN_DELAY:
    subMenuIndex = radio->scan.timeout;
    break;
  case M_SQL_OPEN_T:
    subMenuIndex = radio->sq.openTime;
    break;
  case M_SQL_CLOSE_T:
    subMenuIndex = radio->sq.closeTime;
    break;
  case M_SQL_TO_OPEN:
    subMenuIndex = radio->scan.openedTimeout;
    break;
  case M_SQL_TO_CLOSE:
    subMenuIndex = radio->scan.closedTimeout;
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
  case M_EEPROM_TYPE:
    subMenuIndex = gSettings.eepromType;
    break;
  case M_SKIP_GARBAGE_FREQS:
    subMenuIndex = gSettings.skipGarbageFrequencies;
    break;
  case M_MAIN_APP:
    for (i = 0; i < appsToRunCount; ++i) {
      if (appsAvailableToRun[i]->id == gSettings.mainApp) {
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

void SETTINGS_init() {
  for (uint8_t i = 0; i < MENU_SIZE; ++i) {
    if (menu[i].type == M_MAIN_APP) {
      menu[i].size = appsToRunCount;
      break;
    }
  }
  gRedrawScreen = true;
}

void SETTINGS_update() {}

static void setMenuIndexAndRun(uint16_t v) {
  menuIndex = v - 1;
  setInitialSubmenuIndex();
  isSubMenu = true;
}

bool SETTINGS_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
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
  const MenuItem *item = &menu[menuIndex];
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
    if (item->type == M_NICKNAME) {
      gTextInputSize = 9;
      gTextinputText = gSettings.nickName;
      gTextInputCallback = setNickname;
      APPS_run(APP_TEXTINPUT);
      return true;
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
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  }
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


static App meta = {
    .id = APP_SETTINGS,
    .name = "Settings",
    .init = SETTINGS_init,
    .update = SETTINGS_update,
    .render = SETTINGS_render,
    .key = SETTINGS_key,
};

App *SETTINGS_Meta() { return &meta; }
