#include "apps/apps.h"
#include "board.h"
#include "driver/audio.h"
#include "driver/backlight.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "driver/systick.h"
#include "helper/battery.h"
#include "radio.h"
#include "scheduler.h"
#include "ui/components.h"
#include <stdbool.h>
#include <string.h>

uint8_t previousBatteryLevel = 255;
bool batteryBlink = true;

void _putchar(char c) {}

static void onKey(KEY_Code_t k, bool p, bool h) {
  if (k != KEY_INVALID) {
    BACKLIGHT_On();
    TaskTouch(BACKLIGHT_Update);
  }
  APPS_key(k, p, h);
}

static void UpdateBattery() {
  BATTERY_UpdateBatteryInfo();
  if (gBatteryDisplayLevel) {
    if (previousBatteryLevel != gBatteryDisplayLevel) {
      previousBatteryLevel = gBatteryDisplayLevel;
      UI_Battery(gBatteryDisplayLevel);
      gRedrawStatus = true;
    }
  } else {
    batteryBlink = !batteryBlink;
    if (batteryBlink) {
      UI_Battery(gBatteryDisplayLevel);
    } else {
      memset(gStatusLine + 115, 0, 13);
    }
    gRedrawStatus = true;
  }
}

static void Update() { APPS_update(); }

static void Render() {
  APPS_render();
  ST7565_Render();
}

static void Keys() { KEYBOARD_CheckKeys(onKey); }

void Main(void) {
  SYSTEM_ConfigureSysCon();
  SYSTICK_Init();

  BOARD_Init();

  BATTERY_UpdateBatteryInfo();
  RADIO_SetupRegisters();

  uint32_t f = 43400000;

  RADIO_TuneTo(f, true);
  BK4819_Squelch(3, f);

  BACKLIGHT_SetDuration(15);
  BACKLIGHT_On();
  UpdateBattery();

  TaskAdd("BL", BACKLIGHT_Update, 1000, true);
  TaskAdd("BAT", UpdateBattery, 1000, true);

  APPS_run(APP_STILL);
  TaskAdd("Update", Update, 1, true);
  TaskAdd("Render", Render, 33, true);
  TaskAdd("Keys", Keys, 10, true);

  while (1) {
    TasksUpdate();
  }
}
