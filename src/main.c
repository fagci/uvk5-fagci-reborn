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

void CheckKeys() { KEYBOARD_CheckKeys(APPS_key); }

void Main(void) {
  SYSTEM_ConfigureSysCon();
  SYSTICK_Init();

  BOARD_Init();

  BATTERY_UpdateBatteryInfo();
  RADIO_SetupRegisters();

  uint32_t f = 43400000;

  BK4819_TuneTo(f, true);
  BK4819_Squelch(3, f);

  BACKLIGHT_On();

  TaskAdd(BACKLIGHT_Update, 1000, true);
  TaskAdd(BATTERY_UpdateBatteryInfo, 5000, true);

  TaskAdd(APPS_update, 1, true);
  TaskAdd(APPS_render, 1, true);
  TaskAdd(CheckKeys, 1, true);

  APPS_run(APP_SPECTRUM);

  while (1) {
    SYSTEM_DelayMs(100);
  }
}
