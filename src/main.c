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
#include "external/printf/printf.h"
#include "helper/battery.h"
#include "helper/measurements.h"
#include "inc/dp32g030/gpio.h"
#include "inc/dp32g030/syscon.h"
#include "misc.h"
#include "radio.h"
#include "scheduler.h"
#include "ui/components.h"
#include "ui/helper.h"
#include <stdbool.h>
#include <string.h>

void _putchar(char c) {}

Task *backlightTask;

// Delay to wait for rssi:
// create Task
// wait it to complete

void Delay() {
  for (uint16_t i = 0; i < 20000; i++)
    continue;
}
void onKey(KEY_Code_t k, bool p, bool h) {
  if (k != KEY_INVALID) {
    BACKLIGHT_On();
    TaskTouch(BACKLIGHT_Update);
  }
  APPS_key(k, p, h);
}

void CheckKeys() { KEYBOARD_CheckKeys(onKey); }

void Main(void) {
  SYSTEM_ConfigureSysCon();
  SYSTICK_Init();

  BOARD_Init();

  BATTERY_UpdateBatteryInfo();
  RADIO_SetupRegisters();

  uint32_t f = 43400000;

  BK4819_TuneTo(f, true);
  BK4819_Squelch(3, f);

  BACKLIGHT_SetDuration(5);
  BACKLIGHT_On();

  backlightTask = TaskAdd("BL", BACKLIGHT_Update, 1000, true);
  TaskAdd("BAT", BATTERY_UpdateBatteryInfo, 5000, true);

  APPS_run(APP_TEST);
  TaskAdd("A Upd", APPS_update, 1, true);
  TaskAdd("Delay", Delay, 1000, true);
  TaskAdd("A Rendr", APPS_render, 33, true);
  TaskAdd("A Keys", CheckKeys, 10, true);

  while (1) {
    SYSTEM_DelayMs(1);
  }
}
