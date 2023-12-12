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
#include "helper/presetlist.h"
#include "inc/dp32g030/gpio.h"
#include "misc.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include "ui/components.h"
#include "ui/graphics.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

uint8_t previousBatteryLevel = 255;
bool showBattery = true;

void _putchar(char c) {}

void selfTest() {
  PrintSmall(0, 0, "PRS O:%u SZ:%u", BANDS_OFFSET, PRESET_SIZE);
  PrintSmall(0, 6, "CHN O:%u SZ:%u", CHANNELS_OFFSET, VFO_SIZE);
  PrintSmall(0, 18, "SET O:%u SZ:%u", SETTINGS_OFFSET, SETTINGS_SIZE);
  ST7565_Blit();

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

      gRedrawScreen = true;
    }
  } else {
    showBattery = !showBattery;
    if (showBattery) {
      UI_Battery(gBatteryDisplayLevel);
    } else {
      memset(gFrameBuffer[0] + BATTERY_X, 0, 13);
    }
    gRedrawScreen = true;
  }
}

static void Update() {
  uint32_t now = elapsedMilliseconds;
  APPS_update();
  UART_printf("%u: APPS_update() took %ums\n", elapsedMilliseconds,
              elapsedMilliseconds - now);
  if (!gRedrawScreen) {
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

  APPS_run(APP_VFO);
  TaskAdd("Update", Update, 1, true);
  TaskAdd("Keys", Keys, 10, true);
}

static uint8_t introIndex = 0;
static void Intro() {
  char pb[] = "-\\|/";
  UI_ClearScreen();
  PrintMedium(4, 0 + 12, "OSFW");
  PrintMedium(16, 2 * 8 + 12, "reb0rn");
  PrintMedium(72, 2 * 8 + 12, "%c", pb[introIndex & 3]);
  PrintSmall(96, 46, "by fagci");
  ST7565_Blit();
  if (PRESETS_Load()) {
    RADIO_LoadCurrentVFO();

    BK4819_TuneTo(gCurrentVfo.fRX);
    BK4819_Squelch(3, gCurrentVfo.fRX);
    BK4819_SetModulation(gCurrentVfo.modulation);

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

  BACKLIGHT_SetDuration(BL_TIME_VALUES[gSettings.backlight]);
  BACKLIGHT_SetBrightness(gSettings.brightness);
  BACKLIGHT_On();
  UpdateBattery();

  /* uint32_t start = 65523760;

  while(true) {
  uint32_t now = elapsedMilliseconds + start;
      UI_ClearScreen();
      char c = (now / 1000)%2 ? ':' : ' ';
      PrintBigDigits(32, 32, "%02u%c%02u%c%02u",(now / 1000 / 60 / 60) % 60 ,c,
  (now / 1000 / 60) % 60, c, (now / 1000) % 60); ST7565_Blit();
      SYSTEM_DelayMs(1000);
  } */

  /* PrintMediumEx(LCD_WIDTH - 1 - 18, 24, POS_R, C_FILL, "145.550");
  PrintSmallEx(LCD_WIDTH - 1, 24, POS_R, C_FILL, "00");
  PrintMediumEx(0, 24 + 12, POS_L, C_FILL, ">");
  PrintMediumEx(LCD_WIDTH - 1 - 18, 24 + 12, POS_R, C_FILL, "446.006");
  PrintSmallEx(LCD_WIDTH - 1, 24 + 12, POS_R, C_FILL, "25");
  PrintMediumEx(LCD_WIDTH - 1 - 18, 24 + 24, POS_R, C_FILL, "145.550");
  PrintSmallEx(LCD_WIDTH - 1, 24 + 24, POS_R, C_FILL, "25");
  ST7565_Blit();

  while (true) {
  } */

  if (KEYBOARD_Poll() == KEY_EXIT) {
    APPS_run(APP_RESET);
    TaskAdd("Update", Update, 1, true);
  } else if (KEYBOARD_Poll() == KEY_MENU) {
    // selfTest();
    for (uint8_t i = 0; i < 6; ++i) {
      Preset p = {};
      RADIO_LoadPreset(i, &p);
      PrintSmall(0, 6 * i, "%u", p.band.bounds.start);
    }
  } else {
    TaskAdd("Intro", Intro, 2, true);
  }

  while (true) {
    TasksUpdate();
  }
}
