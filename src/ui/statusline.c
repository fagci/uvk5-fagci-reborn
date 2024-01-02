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

static void eepromRWReset() {
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

void STATUSLINE_update() {
  BATTERY_UpdateBatteryInfo();
  if (gBatteryDisplayLevel) {
    if (previousBatteryLevel != gBatteryDisplayLevel) {
      previousBatteryLevel = gBatteryDisplayLevel;
      UI_Battery(gBatteryDisplayLevel);

      gRedrawScreen = true;
    }
  } else {
    showBattery = !showBattery;
    gRedrawScreen = true;
  }

  if (lastEepromRead != gEepromRead || lastEepromWrite != gEepromWrite) {
    lastEepromRead = gEepromRead;
    lastEepromWrite = gEepromWrite;
    gRedrawScreen = true;
    TaskAdd("EEPROM RW-", eepromRWReset, 500, false);
  }
}

void STATUSLINE_render() {
  UI_ClearStatus();

  const uint8_t BASE_Y = 4;

  DrawHLine(0, 6, LCD_WIDTH, C_FILL);

  if (showBattery) {
    UI_Battery(gBatteryDisplayLevel);
  }

  if (gSettings.upconverter) {
    FillRect(LCD_WIDTH - 45, BASE_Y - 2, 6, 3, C_FILL);
    DrawVLine(LCD_WIDTH - 44, BASE_Y - 4, 2, C_FILL);
    PutPixel(LCD_WIDTH - 41, BASE_Y - 3, C_FILL);
  }

  PrintSmallEx(LCD_WIDTH - 1, BASE_Y, POS_R, C_FILL,
               "%c%c%c",                 //
               gMonitorMode ? 'M' : ' ', // monitor mode
               gEepromRead ? 'R' : ' ',  // eeprom r
               gEepromWrite ? 'W' : ' '  // eeprom w
  );

  if (UART_IsLogEnabled) {
    PrintSmall(BATTERY_W + 1, BASE_Y, statuslineText, "D:%u",
               UART_IsLogEnabled);
  } else if (statuslineText[0] >= 32) {
    PrintSmall(BATTERY_W + 1, BASE_Y, statuslineText);
  }

  // FillRect(0, 0, LCD_WIDTH, 7, C_INVERT);
}
