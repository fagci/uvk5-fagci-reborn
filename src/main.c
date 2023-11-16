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
#include "ui/helper.h"
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
  if (k == KEY_MENU) {
    if (!p && !h) {
      // show vfo menu?
    } else if (p && h) {
      APPS_run(APP_MAINMENU);
      return;
    }
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

// TODO:
// - menu hold in still mode

// static void TX() {
// DEV = 300 for SSB
// }

static void AddTasks() {
  TaskAdd("BL", BACKLIGHT_Update, 1000, true);
  TaskAdd("BAT", UpdateBattery, 1000, true);

  APPS_run(APP_TEXTINPUT);
  TaskAdd("Update", Update, 1, true);
  TaskAdd("Render", Render, 33, true);
  TaskAdd("Keys", Keys, 10, true);
}

static uint8_t introIndex = 0;
static void Intro() {
  char pb[] = "-\\|/";
  char String[2] = {0};
  sprintf(String, "%c", pb[introIndex & 3]);
  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
  UI_PrintString("OSFW", 4, 4, 0);
  UI_PrintString("reb0rn", 16, 16, 2);
  UI_PrintString(String, 72, 72, 2);
  UI_PrintStringSmallest("by fagci", 96, 46, false, true);
  ST7565_BlitFullScreen();
  if (introIndex++ > 50) {
    AddTasks();
    TaskRemove(Intro);
  }
}

void Main(void) {
  SYSTICK_Init();
  SYSTEM_ConfigureSysCon();

  BOARD_Init();

  BATTERY_UpdateBatteryInfo();
  RADIO_SetupRegisters();

  uint32_t f = 43400000;

  gCurrentVfo.bw = BK4819_FILTER_BW_WIDE;
  gCurrentVfo.step = STEP_25_0kHz;

  RADIO_TuneTo(f, true);
  BK4819_Squelch(3, f);
  BK4819_SetModulation(MOD_FM);

  BACKLIGHT_SetDuration(15);
  BACKLIGHT_On();
  UpdateBattery();

  TaskAdd("Intro", Intro, 20, true);

  while (true) {
    TasksUpdate();
  }
}
