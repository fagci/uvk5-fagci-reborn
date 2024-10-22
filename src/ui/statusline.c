#include "statusline.h"
#include "../driver/eeprom.h"
#include "../driver/si473x.h"
#include "../driver/st7565.h"
#include "../helper/battery.h"
#include "../scheduler.h"
#include "../svc.h"
#include "components.h"
#include "graphics.h"
#include <string.h>

static uint8_t previousBatteryLevel = 255;
static bool showBattery = true;

static bool lastEepromWrite = false;

static char statuslineText[32] = {0};

static void eepromRWReset(void) {
  lastEepromWrite = gEepromWrite = false;
  gRedrawScreen = true;
}

void STATUSLINE_SetText(const char *pattern, ...) {
  char statuslineTextNew[32] = {0};
  va_list args;
  va_start(args, pattern);
  vsnprintf(statuslineTextNew, 31, pattern, args);
  va_end(args);
  if (strcmp(statuslineText, statuslineTextNew)) {
    strncpy(statuslineText, statuslineTextNew, 31);
    gRedrawScreen = true;
  }
}

void STATUSLINE_update(void) {
  BATTERY_UpdateBatteryInfo();
  uint8_t level = gBatteryPercent / 10;
  if (gBatteryPercent < BAT_WARN_PERCENT) {
    showBattery = !showBattery;
    gRedrawScreen = true;
  } else {
    showBattery = true;
  }
  if (previousBatteryLevel != level) {
    previousBatteryLevel = level;
    gRedrawScreen = true;
  }

  if (lastEepromWrite != gEepromWrite) {
    lastEepromWrite = gEepromWrite;
    gRedrawScreen = true;
    TaskAdd("EEPROM RW-", eepromRWReset, 500, false, 0);
  }
}

void STATUSLINE_render(void) {
  UI_ClearStatus();

  const uint8_t BASE_Y = 4;

  DrawHLine(0, 6, LCD_WIDTH, C_FILL);

  if (showBattery) {
    if (gSettings.batteryStyle == BAT_PERCENT ||
        gSettings.batteryStyle == BAT_VOLTAGE) {
      PrintSmallEx(LCD_WIDTH - 1, BASE_Y, POS_R, C_INVERT, "%u%%",
                   gBatteryPercent);
    } else {
      UI_Battery(previousBatteryLevel);
    }
  }

  if (gSettings.batteryStyle == BAT_VOLTAGE) {
    PrintSmallEx(LCD_WIDTH - 1 - 16, BASE_Y, POS_R, C_FILL, "%u.%02uV",
                 gBatteryVoltage / 100, gBatteryVoltage % 100);
  }

  char icons[8] = {'\0'};
  uint8_t idx = 0;

  if (gEepromWrite) {
    icons[idx++] = SYM_EEPROM_W;
  }

  if (SVC_Running(SVC_BEACON)) {
    icons[idx++] = SYM_BEACON;
  }

  if (SVC_Running(SVC_FC)) {
    icons[idx++] = SYM_FC;
  }

  if (SVC_Running(SVC_SCAN)) {
    icons[idx++] = SYM_SCAN;
  }

  if (gMonitorMode) {
    icons[idx++] = SYM_MONITOR;
  }

  if (RADIO_GetRadio() == RADIO_BK1080 || isSi4732On) {
    icons[idx++] = SYM_BROADCAST;
  }

  if (gSettings.upconverter) {
    icons[idx++] = SYM_CONVERTER;
  }

  if (gSettings.noListen &&
      (gCurrentApp == APP_SPECTRUM || gCurrentApp == APP_ANALYZER)) {
    icons[idx++] = SYM_NO_LISTEN;
  }

  if (gSettings.keylock) {
    icons[idx++] = SYM_LOCK;
  }

  if ((gCurrentApp == APP_SAVECH ||
       gCurrentApp == APP_VFO1 || gCurrentApp == APP_VFO2 ||
       gCurrentApp == APP_SPECTRUM)) {
    if (gSettings.currentScanlist == 15) {
      PrintSmallEx(LCD_XCENTER, BASE_Y, POS_C, C_FILL, "SL all");
    } else {
      PrintSmallEx(LCD_XCENTER, BASE_Y, POS_C, C_FILL, "SL %d",
                   gSettings.currentScanlist + 1);
    }
  }

  PrintSymbolsEx(LCD_WIDTH - 1 -
                     (gSettings.batteryStyle == BAT_VOLTAGE ? 38 : 18),
                 BASE_Y, POS_R, C_FILL, "%s", icons);

  PrintSmall(0, BASE_Y, statuslineText);

  // FillRect(0, 0, LCD_WIDTH, 7, C_INVERT);
}
