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
#include "ui/statusline.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void _putchar(char c) {}

void selfTest() {
  PrintSmall(0, 0, "PRS O:%u SZ:%u", BANDS_OFFSET, PRESET_SIZE);
  PrintSmall(0, 6, "CHN O:%u SZ:%u", CHANNELS_OFFSET, VFO_SIZE);
  PrintSmall(0, 18, "SET O:%u SZ:%u", SETTINGS_OFFSET, SETTINGS_SIZE);
  ST7565_Blit();

  while (true)
    continue;
}

/*

Запускается меню повторно

Похоже потому что после запуска приложения ещё раз обрабатывается клавиша?

*/

static void onKey(KEY_Code_t key, bool pressed, bool hold) {
  if (key != KEY_INVALID) {
    BACKLIGHT_On();
    TaskTouch(BACKLIGHT_Update);
  }

  if (APPS_key(key, pressed, hold)) {
    gRedrawScreen = true;
    return;
  }

  if (key == KEY_MENU) {
    if (pressed) {
      if (hold) {
        APPS_run(APP_SETTINGS);
        return;
      }
    } else {
      if (!hold) {
        APPS_run(APP_APPS_LIST);
        return;
      }
    }
  }

  if (key == KEY_SIDE2 && !hold && pressed) {
    GPIO_FlipBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
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
  STATUSLINE_render();
  APPS_render();
  UART_printf("%u: APPS_render() took %ums\n", elapsedMilliseconds,
              elapsedMilliseconds - now);
  now = elapsedMilliseconds;
  ST7565_Render();
  UART_printf("%u: ST7565_Render() took %ums\n", elapsedMilliseconds,
              elapsedMilliseconds - now);
}

static void Keys() { KEYBOARD_CheckKeys(onKey); }

static void sysUpdate() {
  Keys();
  STATUSLINE_update();
  BACKLIGHT_Update();
}

// TODO:
// - menu hold in still mode

// static void TX() {
// DEV = 300 for SSB
// SAVE 74, dev
// }

static void AddTasks() {
  TaskAdd("1s sys upd", sysUpdate, 1000, true);

  APPS_run(APP_STILL);
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

    BK4819_TuneTo(gCurrentVFO->fRX);
    BK4819_Squelch(3, gCurrentVFO->fRX);
    BK4819_SetModulation(gCurrentVFO->modulation);

    TaskRemove(Intro);
    AddTasks();
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
