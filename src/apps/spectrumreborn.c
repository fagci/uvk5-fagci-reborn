#include "spectrumreborn.h"
#include "../driver/audio.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include "apps.h"
#include "finput.h"
#include <string.h>

#define BLACKLIST_SIZE 32
#define HISTORY_SIZE 128

static const uint8_t S_HEIGHT = 32;
static const uint8_t SPECTRUM_Y = 16;
static const uint8_t S_BOTTOM = SPECTRUM_Y + S_HEIGHT;

static uint8_t i = 0;
static uint32_t f = 0;
static uint32_t fStart, fEnd;
static uint8_t rssiMin, rssiMax;

static uint16_t rssiHistory[128] = {0};
static uint8_t msmTime = 3;
static uint32_t currentFreq;

static bool gettingRssi = false;

static void resetRssiHistory() { memset(rssiHistory, 0, HISTORY_SIZE); }

static void updateScanStats() {
  uint8_t rssi;
  rssiMin = rssiMax = rssiHistory[0];
  for (uint8_t i = 1; i < 128; ++i) {
    rssi = rssiHistory[i];
    if (rssi < rssiMin) {
      rssiMin = rssi;
    }
    if (rssi > rssiMax) {
      rssiMax = rssi;
    }
  }
}

static void writeRssi() {
  rssiHistory[i++] = BK4819_GetRSSI();
  f += StepFrequencyTable[gCurrentVfo.step];
  gettingRssi = false;
  // BK4819_TuneTo(f, true);
  BK4819_SetFrequency(f); // need to test
}

static void step() {
  gettingRssi = true;
  BK4819_TuneTo(f, true);
  TaskAdd("Get RSSI", writeRssi, msmTime, false)->priority = 0;
}

static void startNewScan() {
  i = 0;

  f = fStart = currentFreq;
  fEnd = currentFreq + 128 * StepFrequencyTable[gCurrentVfo.step];

  resetRssiHistory();

  step();
}

bool SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed) {
    return false;
  }
  switch (Key) {
  case KEY_EXIT:
    APPS_exit();
    return true;
  case KEY_5:
    gFInputValue = &gCurrentVfo.fRX;
    APPS_run(APP_FINPUT);
    return true;
  default:
    break;
  }
  return false;
}

void SPECTRUM_init(void) {
  BK4819_WriteRegister(0x43, 0b0000000110111100);
  currentFreq = gCurrentVfo.fRX;
  startNewScan();
}

void SPECTRUM_update(void) {
  if (f >= fEnd) {
    gRedrawScreen = true;
    return;
  }
  if (gettingRssi) {
    return;
  }
  step();
}

void SPECTRUM_render(void) {
  updateScanStats();
  UI_ClearScreen();
  UI_FSmall(currentFreq);

  uint16_t vMin = rssiMin - 2;
  uint16_t vMax = rssiMax + 20 + (rssiMax - rssiMin) / 2;

  for (uint8_t x = 0; x < LCD_WIDTH; ++x) {
    uint8_t yVal = ConvertDomain(rssiHistory[x], vMin, vMax, 0, S_HEIGHT);
    DrawHLine(S_BOTTOM - yVal, S_BOTTOM, x, true);
  }

  startNewScan(); // bad practice, but we need to render before
}
