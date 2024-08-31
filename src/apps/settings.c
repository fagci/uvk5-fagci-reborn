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
  M_DW,
  M_SCRAMBLER,
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
  M_SI4732_POWER_OFF,
  M_ROGER,
  M_TONE_LOCAL,
  M_STE,
  M_PTT_LOCK,
  M_DTMF_DECODE,
  M_RESET,
} Menu;

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static char Output[16];

static const char *fltBound[] = {"240MHz", "280MHz"};

static const uint16_t BAT_CAL_MIN = 1900;
// static const uint16_t BAT_CAL_MAX = 2155;

static const MenuItem menu[] = {
    {"Upconv", M_UPCONVERTER, ARRAY_SIZE(upConverterFreqNames)},
    {"Main app", M_MAIN_APP, ARRAY_SIZE(appsAvailableToRun)},
    {"SQL open time", M_SQL_OPEN_T, 7},
    {"SQL close time", M_SQL_CLOSE_T, 3},
    {"SCAN single freq time", M_SCAN_DELAY, 255},
    {"SCAN listen time", M_SQL_TO_OPEN, ARRAY_SIZE(SCAN_TIMEOUT_NAMES)},
    {"SCAN after close time", M_SQL_TO_CLOSE, ARRAY_SIZE(SCAN_TIMEOUT_NAMES)},
    {"Dual watch", M_DW, 3},
    {"Brightness", M_BRIGHTNESS, 16},
    {"Contrast", M_CONTRAST, 16},
    {"BL time", M_BL_TIME, ARRAY_SIZE(BL_TIME_VALUES)},
    {"BL SQL mode", M_BL_SQL, ARRAY_SIZE(BL_SQL_MODE_NAMES)},
    {"Filter bound", M_FLT_BOUND, 2},
    {"Scrambler", M_SCRAMBLER, 16},
    {"Beep", M_BEEP, 2},
    {"BAT calibration", M_BAT_CAL, 255},
    {"BAT type", M_BAT_TYPE, ARRAY_SIZE(BATTERY_TYPE_NAMES)},
    {"BAT style", M_BAT_STYLE, ARRAY_SIZE(BATTERY_STYLE_NAMES)},
    {"EEPROM type", M_EEPROM_TYPE, ARRAY_SIZE(EEPROM_TYPE_NAMES)},
    {"Nickname", M_NICKNAME, 0},
    {"Skip garbage freqs", M_SKIP_GARBAGE_FREQS, 2},
    {"SI4732 power off", M_SI4732_POWER_OFF, 2},
    {"Roger", M_ROGER, 4},
    {"Tone local", M_TONE_LOCAL, 2},
    {"STE", M_STE, 2},
    {"Lock PTT", M_PTT_LOCK, 2},
    {"DTMF decode", M_PTT_LOCK, 2},
    {"EEPROM reset", M_RESET, 2},
};

static const uint8_t MENU_SIZE = ARRAY_SIZE(menu);

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
  case M_SQL_CLOSE_T:
    sprintf(name, "%ums", index * 5);
    return;
  case M_SQL_TO_OPEN:
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
  case M_SCRAMBLER:
    sprintf(name, "%u", index);
    return;
  case M_CONTRAST:
    sprintf(name, "%d", index - 8);
    return;
  case M_BL_TIME:
    strncpy(name, BL_TIME_NAMES[index], 31);
    return;
  case M_BEEP:
  case M_STE:
  case M_DTMF_DECODE:
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
  case M_EEPROM_TYPE:
    strncpy(name, EEPROM_TYPE_NAMES[index], 31);
    return;
  case M_RESET:
  case M_TONE_LOCAL:
  case M_PTT_LOCK:
  case M_SKIP_GARBAGE_FREQS:
    strncpy(name, yesNo[index], 31);
    return;
  case M_SI4732_POWER_OFF:
  case M_DW:
    strncpy(name, dwNames[index], 31);
    return;
  case M_ROGER:
    strncpy(name, rogerNames[index], 31);
    return;
  case M_NICKNAME:
    strncpy(name, gSettings.nickName, 31);
    return;
  default:
    break;
  }
}

static void setNickname(void) {
  strncpy(gSettings.nickName, gTextinputText, gTextInputSize);
  SETTINGS_Save();
}

static void accept(void) {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_UPCONVERTER: {
    uint32_t f = GetScreenF(radio->rx.f);
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
  case M_CONTRAST:
    gSettings.contrast = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_BL_TIME:
    gSettings.backlight = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_SCRAMBLER:
    gSettings.scrambler = subMenuIndex;
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
  case M_SI4732_POWER_OFF:
    gSettings.si4732PowerOff = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_DW:
    gSettings.dw = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_ROGER:
    gSettings.roger = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_STE:
    gSettings.ste = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_TONE_LOCAL:
    gSettings.toneLocal = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_PTT_LOCK:
    gSettings.pttLock = subMenuIndex;
    SETTINGS_Save();
    break;
  case M_DTMF_DECODE:
    gSettings.dtmfdecode = subMenuIndex;
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
  case M_SCRAMBLER:
    sprintf(Output, "%u", gSettings.scrambler);
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
  case M_EEPROM_TYPE:
    return EEPROM_TYPE_NAMES[gSettings.eepromType];
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
  case M_DTMF_DECODE:
    return onOff[gSettings.dtmfdecode];
  case M_STE:
    return onOff[gSettings.ste];
  case M_SKIP_GARBAGE_FREQS:
    return yesNo[gSettings.skipGarbageFrequencies];
  case M_SI4732_POWER_OFF:
    return yesNo[gSettings.si4732PowerOff];
  case M_DW:
    return dwNames[gSettings.dw];
  case M_TONE_LOCAL:
    return yesNo[gSettings.toneLocal];
  case M_PTT_LOCK:
    return yesNo[gSettings.pttLock];
  case M_ROGER:
    return rogerNames[gSettings.roger];
  case M_NICKNAME:
    return gSettings.nickName;
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
  case M_SCRAMBLER:
    gSettings.scrambler = subMenuIndex;
    RADIO_SetupByCurrentVFO();
    break;
  case M_CONTRAST:
    gSettings.contrast = subMenuIndex;
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
  case M_CONTRAST:
    subMenuIndex = gSettings.contrast;
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
  case M_SCRAMBLER:
    subMenuIndex = gSettings.scrambler;
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
  case M_SI4732_POWER_OFF:
    subMenuIndex = gSettings.si4732PowerOff;
    break;
  case M_DW:
    subMenuIndex = gSettings.dw;
    break;
  case M_ROGER:
    subMenuIndex = gSettings.roger;
    break;
  case M_TONE_LOCAL:
    subMenuIndex = gSettings.toneLocal;
    break;
  case M_STE:
    subMenuIndex = gSettings.ste;
    break;
  case M_DTMF_DECODE:
    subMenuIndex = gSettings.dtmfdecode;
    break;
  case M_PTT_LOCK:
    subMenuIndex = gSettings.pttLock;
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

static void setMenuIndexAndRun(uint16_t v) {
  menuIndex = v - 1;
  setInitialSubmenuIndex();
  isSubMenu = true;
}

bool SETTINGS_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
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

void SETTINGS_render(void) {
  UI_ClearScreen();
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  } else {
    STATUSLINE_SetText(apps[APP_SETTINGS].name);
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
