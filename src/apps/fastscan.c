#include "fastscan.h"
#include "../driver/bk4819.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/graphics.h"

static uint8_t lastUpdate = 0;
static uint32_t gotF = 0;
static bool isDone = false;
static uint8_t hits = 0;

void FASTSCAN_init() {
  BK4819_StopScan();
  BK4819_DisableFilter();
  BK4819_EnableFrequencyScan();
}
void FASTSCAN_update() {
  if (elapsedMilliseconds - lastUpdate >= 10) {
    lastUpdate = elapsedMilliseconds;
    uint32_t f = 0;

    if (BK4819_GetFrequencyScanResult(&f)) {
      int32_t d = f - gotF;
      if (d < 0) {
        d = -d;
      }
      if (d < 100 && hits++ > 3) {
        isDone = true;
        gRedrawScreen = true;
      }
      BK4819_DisableFrequencyScan();
      BK4819_EnableFrequencyScan(); // continue
      gotF = f;
    }
  }
}
bool FASTSCAN_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  // Simple keypress
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_MENU:
      RADIO_TuneToSave(gotF);
      APPS_exit();
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
  PrintMediumEx(LCD_WIDTH / 2, LCD_HEIGHT / 2, POS_C, C_FILL, "Scanning...");
  if (isDone) {
    PrintMediumEx(LCD_WIDTH / 2, LCD_HEIGHT / 2 + 8, POS_C, C_FILL, "%u.%05u",
                  gotF / 100000, gotF % 100000);
  }
}
void FASTSCAN_deinit() {
  BK4819_StopScan();
  RADIO_SetupByCurrentVFO();
}
