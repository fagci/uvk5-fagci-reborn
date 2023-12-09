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
#include "driver/uart.h"
#include "helper/battery.h"
#include "inc/dp32g030/gpio.h"
#include "misc.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include "ui/components.h"
#include "ui/graphics.h"
#include "ui/helper.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

uint8_t previousBatteryLevel = 255;
bool showBattery = true;

void _putchar(char c) {}

void selfTest() {
  UI_PrintSmallest(0, 0, "PRS O:%u SZ:%u", BANDS_OFFSET, PRESET_SIZE);
  UI_PrintSmallest(0, 6, "CHN O:%u SZ:%u", CHANNELS_OFFSET, VFO_SIZE);
  UI_PrintSmallest(0, 18, "SET O:%u SZ:%u", SETTINGS_OFFSET, SETTINGS_SIZE);
  ST7565_BlitFullScreen();

  while (true)
    continue;
}

static void onKey(KEY_Code_t key, bool pressed, bool hold) {
  if (key != KEY_INVALID) {
    BACKLIGHT_On();
    TaskTouch(BACKLIGHT_Update);
  }
  if (APPS_key(key, pressed, hold)) {
    gRedrawScreen = true;
    return;
  }
  if (key == KEY_MENU && gCurrentApp != APP_SETTINGS &&
      gCurrentApp != APP_MAINMENU) {
    if (hold && pressed) {
      APPS_run(APP_SETTINGS);
      return;
    }
    if (!hold && !pressed) {
      APPS_run(APP_MAINMENU);
      return;
    }
  }
  if (key == KEY_SIDE2 && !hold && pressed) {
    GPIO_FlipBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
  }
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
    showBattery = !showBattery;
    if (showBattery) {
      UI_Battery(gBatteryDisplayLevel);
    } else {
      memset(gStatusLine + BATTERY_X, 0, 13);
    }
    gRedrawStatus = true;
  }
}

static void Update() {
  uint32_t now = elapsedMilliseconds;
  APPS_update();
  UART_printf("%u: APPS_update() took %ums\n", elapsedMilliseconds,
              elapsedMilliseconds - now);
  if (!gRedrawStatus && !gRedrawScreen) {
    return;
  }
  now = elapsedMilliseconds;
  APPS_render();
  UART_printf("%u: APPS_render() took %ums\n", elapsedMilliseconds,
              elapsedMilliseconds - now);
  now = elapsedMilliseconds;
  ST7565_Render();
  UART_printf("%u: ST7565_Render() took %ums\n", elapsedMilliseconds,
              elapsedMilliseconds - now);
}

static void Keys() { KEYBOARD_CheckKeys(onKey); }

// TODO:
// - menu hold in still mode

// static void TX() {
// DEV = 300 for SSB
// SAVE 74, dev
// }

static void AddTasks() {
  TaskAdd("BL", BACKLIGHT_Update, 1000, true);
  TaskAdd("BAT", UpdateBattery, 1000, true);

  APPS_run(APP_SPECTRUM);
  TaskAdd("Update", Update, 1, true);
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
  UI_PrintSmallest(96, 46, "by fagci");
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
  BACKLIGHT_Toggle(true);

  SETTINGS_Load();

  UART_Init();

  BATTERY_UpdateBatteryInfo();
  RADIO_SetupRegisters();

  RADIO_LoadCurrentVFO();

  BK4819_TuneTo(gCurrentVfo.fRX);
  BK4819_Squelch(3, gCurrentVfo.fRX);
  BK4819_SetModulation(gCurrentVfo.modulation);

  BACKLIGHT_SetDuration(BL_TIME_VALUES[gSettings.backlight]);
  BACKLIGHT_SetBrightness(gSettings.brightness);
  BACKLIGHT_On();
  UpdateBattery();

#include "ui/fonts/DSEG7_Classic_Mini_Regular_11.h"
#include "ui/fonts/Dialog_plain_5.h"
#include "ui/fonts/Serif_plain_7.h"
  const char *text = "145.550";
  moveTo(2, 12);
  for (uint8_t i = 0; i < strlen(text); i++) {
    write(text[i], 1, 1, false, true, false, &DSEG7_Classic_Mini_Regular_11);
  }
  const char *text2 =
      "Test string to check wraps and another possibilities of new library";
  moveTo(2, 32);
  for (uint8_t i = 0; i < strlen(text2); i++) {
    write(text2[i], 1, 1, true, true, false, &Serif_plain_7);
  }

  ST7565_BlitFullScreen();
  while (true) {
  }

  if (KEYBOARD_Poll() == KEY_EXIT) {
    APPS_run(APP_RESET);
    TaskAdd("Update", Update, 1, true);
  } else if (KEYBOARD_Poll() == KEY_MENU) {
    // selfTest();
    for (uint8_t i = 0; i < 6; ++i) {
      Preset p = {};
      RADIO_LoadPreset(i, &p);
      UI_PrintSmallest(0, 6 * i, "%u", p.band.bounds.start);
    }
  } else {
    TaskAdd("Intro", Intro, 2, true);
  }

  while (true) {
    TasksUpdate();
  }
}
