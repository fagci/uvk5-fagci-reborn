#include "fastscan.h"
#include "../driver/bk4819.h"
#include "../radio.h"
#include "../ui/graphics.h"
#include "apps.h"

static uint32_t scanF = 0;
static uint32_t gotF = 0;
static bool isDone = false;
static uint8_t hits = 0;

static uint32_t delta(uint32_t f1, uint32_t f2) {
  int64_t d = f1 - f2;
  if (d < 0) {
    d = -d;
  }
  return d;
}

void FASTSCAN_init() {
  BK4819_StopScan();
  BK4819_DisableFilter();
  BK4819_EnableFrequencyScanEx(F_SC_T_0_2s);
}

void FASTSCAN_update() {
  uint32_t f = 0;

  if (BK4819_GetFrequencyScanResult(&f)) {
    if (delta(f, scanF) < 100) {
      if (hits++ >= 3) {
        isDone = true;
        gotF = scanF;
        gRedrawScreen = true;
      }
    } else {
      hits = 0;
      isDone = false;
    }
    BK4819_DisableFrequencyScan();
    BK4819_EnableFrequencyScanEx(F_SC_T_0_2s);
    scanF = f;
  }
}

bool FASTSCAN_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  // Simple keypress
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_MENU:
      if (gotF > 0) {
        RADIO_TuneToSave(scanF);
        APPS_exit();
      }
      return true;
    case KEY_EXIT:
      APPS_exit();
      return true;
    default:
      break;
    }
  }
  return false;
}

void FASTSCAN_render() {
  UI_ClearScreen();
  PrintMediumEx(LCD_XCENTER, LCD_HEIGHT / 2, POS_C, C_FILL, "Scanning...");
  if (gotF > 0) {
    PrintMediumEx(LCD_XCENTER, LCD_HEIGHT / 2 + 8, POS_C, C_FILL, "%u.%05u",
                  gotF / 100000, gotF % 100000);
  }
}

void FASTSCAN_deinit() {
  BK4819_StopScan();
  BK4819_EnableRX();
}


static App meta = {
    .id = APP_FASTSCAN,
    .name = "FASTSCAN",
    .runnable = true,
    .init = FASTSCAN_init,
    .update = FASTSCAN_update,
    .render = FASTSCAN_render,
    .key = FASTSCAN_key,
    .deinit = FASTSCAN_deinit,
};

App *FASTSCAN_Meta() { return &meta; }
