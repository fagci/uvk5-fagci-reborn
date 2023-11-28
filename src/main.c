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
#include "inc/dp32g030/gpio.h"
#include "misc.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include "ui/components.h"
#include "ui/helper.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

uint8_t previousBatteryLevel = 255;
bool showBattery = true;

void _putchar(char c) {}

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
      memset(gStatusLine + 115, 0, 13);
    }
    gRedrawStatus = true;
  }
}

static void Update() { APPS_update(); }

static void Render() {
  if (!gRedrawStatus && !gRedrawScreen) {
    return;
  }
  APPS_render();
  ST7565_Render();
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

  BATTERY_UpdateBatteryInfo();
  RADIO_SetupRegisters();

  RADIO_LoadCurrentVFO();

  if (gCurrentVfo.fRX < F_MIN || gCurrentVfo.fRX > F_MAX) {
    gCurrentVfo.fRX = 43400000;
    gCurrentVfo.bw = BK4819_FILTER_BW_WIDE;
    gCurrentVfo.step = STEP_25_0kHz;
    gCurrentVfo.modulation = MOD_FM;
    gCurrentVfo.codeTypeTx = 3;
    gCurrentVfo.squelch = 4;
    gCurrentVfo.squelchType = SQUELCH_RSSI_NOISE_GLITCH;
    RADIO_SaveCurrentVFO();
  }

  if (gSettings.upconverter > UPCONVERTER_125M) {
    gSettings.upconverter = UPCONVERTER_OFF;
    gSettings.squelch = 4;
    gSettings.spectrumAutosquelch = false;
    gSettings.backlight = 15;
    gSettings.brightness = 9;
    gSettings.batsave = 4;
    gSettings.beep = 1;
    gSettings.busyChannelTxLock = false;
    gSettings.keylock = false;
    gSettings.crossBand = false;
    gSettings.currentScanlist = 0;
    gSettings.dw = false;
    gSettings.repeaterSte = true;
    gSettings.ste = true;
    gSettings.scrambler = 0;
    gSettings.vox = false;
    gSettings.dtmfdecode = false;

    gSettings.chDisplayMode = 0; // TODO: update
    gSettings.scanmode = 0;      // TODO: update
    gSettings.micGain = 15;      // TODO: update
    gSettings.roger = 1;         // TODO: update
    gSettings.txTime = 0;        // TODO: update

    SETTINGS_Save();
  }

  BK4819_TuneTo(gCurrentVfo.fRX, true);
  BK4819_Squelch(3, gCurrentVfo.fRX);
  BK4819_SetModulation(gCurrentVfo.modulation);

  BACKLIGHT_SetDuration(gSettings.backlight);
  BACKLIGHT_SetBrightness(gSettings.brightness);
  BACKLIGHT_On();
  UpdateBattery();

  UI_PrintSmallest(0, 0, "PRS O:%u SZ:%u", BANDS_OFFSET, PRESET_SIZE);
  UI_PrintSmallest(0, 6, "CHN O:%u SZ:%u", CHANNELS_OFFSET, VFO_SIZE);
  UI_PrintSmallest(0, 12, "CUR O:%u SZ:%u", CURRENT_VFO_OFFSET,
                   CURRENT_VFO_SIZE);
  UI_PrintSmallest(0, 18, "SET O:%u SZ:%u", SETTINGS_OFFSET, SETTINGS_SIZE);
  ST7565_BlitFullScreen();

  gSettings.brightness = 4;
  SETTINGS_Save();
  gSettings.brightness = 0;
  SETTINGS_Load();
  if (gSettings.brightness != 4) {
    while (true) {
      GPIO_FlipBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
      SYSTEM_DelayMs(200);
    }
  }

  gCurrentVfo.step = 5;
  RADIO_SaveCurrentVFO();
  gCurrentVfo.step = 0;
  RADIO_LoadCurrentVFO();
  if (gCurrentVfo.step != 5) {
    while (true) {
      GPIO_FlipBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
      SYSTEM_DelayMs(200);
      GPIO_FlipBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
      SYSTEM_DelayMs(100);
      GPIO_FlipBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
      SYSTEM_DelayMs(300);
    }
  }

  TaskAdd("Intro", Intro, 20, true);

  while (true) {
    TasksUpdate();
  }
}
