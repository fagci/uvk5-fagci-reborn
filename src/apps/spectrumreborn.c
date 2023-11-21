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

static uint16_t rssiHistory[128] = {0};
static uint8_t i = 0;
static uint8_t msmTime = 2;
static uint32_t currentFreq;
static uint32_t fStart, fEnd, f;
static bool gettingRssi = false;

static void resetRssiHistory() { memset(rssiHistory, 0, HISTORY_SIZE); }
static void writeRssi() {
  rssiHistory[i++] = BK4819_GetRSSI();
  f += StepFrequencyTable[gCurrentVfo.step];
  gettingRssi = false;
  BK4819_TuneTo(f, true);
}

static void step() {
  gettingRssi = true;
  BK4819_TuneTo(f, true);
  TaskAdd("Get RSSI", writeRssi, msmTime, false);
  TaskSetPriority(writeRssi, 0);
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

static void render() {
  UI_ClearScreen();
  UI_FSmall(currentFreq);
  for (uint8_t x = 0; x < LCD_WIDTH; ++x) {
    uint32_t f = 43400000;
    bool isVhf = f >= 3000000;
    uint16_t rssi = rssiHistory[x];
    uint16_t rssiMin = isVhf ? 38 : 78;
    uint16_t rssiMax = isVhf ? 134 : 174;
    DrawHLine(40 - ConvertDomain(rssi, rssiMin, rssiMax, 0, 39), 40, x, true);
  }
  gRedrawScreen = true;
}

void SPECTRUM_update(void) {
  if (f >= fEnd) {
    render();
    startNewScan();
    return;
  }
  if (!gettingRssi) {
    step();
  }
}

void SPECTRUM_render(void) {}
