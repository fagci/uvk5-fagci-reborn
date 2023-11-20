#include "spectrumreborn.h"
#include "../driver/audio.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include <string.h>

#define BLACKLIST_SIZE 32
#define HISTORY_SIZE 128

static uint16_t rssiHistory[128] = {0};
static uint8_t i = 0;
static uint8_t msmTime = 2;
static uint32_t currentFreq;
static uint32_t fStart, fEnd;
static bool gettingRssi = false;

static void resetRssiHistory() { memset(rssiHistory, 0, HISTORY_SIZE); }
static void measure() {
  rssiHistory[i++] = BK4819_GetRSSI();
  gCurrentVfo.fRX += StepFrequencyTable[gCurrentVfo.step];
  gettingRssi = false;
  BK4819_TuneTo(gCurrentVfo.fRX, true);
}

static void getRssi() {
  gettingRssi = true;
  BK4819_TuneTo(gCurrentVfo.fRX, true);
  TaskAdd("Get RSSI", measure, msmTime, false);
}

static void startNewScan() {
  i = 0;

  fStart = currentFreq;
  fEnd = currentFreq + 128 * StepFrequencyTable[gCurrentVfo.step];

  gCurrentVfo.fRX = fStart;

  resetRssiHistory();

  getRssi();
}

bool SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  return false;
}

void SPECTRUM_init(void) {
  currentFreq = gCurrentVfo.fRX;
  gettingRssi = true;
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
  if (gCurrentVfo.fRX >= fEnd) {
    render();
    startNewScan();
    return;
  }
  if (!gettingRssi) {
    getRssi();
  }
}

void SPECTRUM_render(void) {}
