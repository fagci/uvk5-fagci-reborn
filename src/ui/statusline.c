#include "statusline.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../driver/uart.h"
#include "../helper/battery.h"
#include "../scheduler.h"
#include "components.h"
#include "graphics.h"
#include <string.h>

static uint8_t previousBatteryLevel = 255;
static bool showBattery = true;

static bool lastEepromRead = false;
static bool lastEepromWrite = false;

static char statuslineText[32] = {0};

static void eepromRWReset(void) {
  lastEepromRead = lastEepromWrite = gEepromRead = gEepromWrite = false;
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
  }
  if (previousBatteryLevel != level) {
    previousBatteryLevel = level;
    gRedrawScreen = true;
  }

  if (lastEepromRead != gEepromRead || lastEepromWrite != gEepromWrite) {
    lastEepromRead = gEepromRead;
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
    /* } else if (gEepromRead) {
      icons[idx++] = SYM_EEPROM_R; */
  }

  if (gMonitorMode) {
    icons[idx++] = SYM_MONITOR;
  }

  if (isBK1080) {
    icons[idx++] = SYM_BROADCAST;
  }

  if (gSettings.upconverter) {
    icons[idx++] = SYM_CONVERTER;
  }

  PrintSymbolsEx(LCD_WIDTH - 1 -
                     (gSettings.batteryStyle == BAT_VOLTAGE ? 38 : 18),
                 BASE_Y, POS_R, C_FILL, "%s", icons);

  if (UART_IsLogEnabled) {
    PrintSmall(0, BASE_Y, statuslineText, "D:%u", UART_IsLogEnabled);
  } else if (statuslineText[0] >= 32) {
    PrintSmall(0, BASE_Y, statuslineText);
  }

  // FillRect(0, 0, LCD_WIDTH, 7, C_INVERT);
}
