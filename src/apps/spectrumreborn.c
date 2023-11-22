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

static const uint8_t S_HEIGHT = 32;
static const uint8_t SPECTRUM_Y = 16;
static const uint8_t S_BOTTOM = SPECTRUM_Y + S_HEIGHT;

// static uint8_t x = 0;
static uint32_t f;
static uint16_t rssiHistory[LCD_WIDTH] = {0};

static Band bandsToScan[32] = {0};
static uint8_t bandsCount = 0;
static uint8_t currentBandIndex = 0;
static Band *currentBand;

static uint16_t currentStepSize;
static uint8_t exLen;
static uint16_t stepsCount;

static uint16_t currentStep;

static uint8_t msmTime = 1;

static bool gettingRssi = false;

static void resetRssiHistory() { memset(rssiHistory, 0, LCD_WIDTH); }

static uint8_t ceilDiv(uint16_t a, uint16_t b) { return (a + b - 1) / b; }

static void writeRssi() {
  uint8_t rssi = BK4819_GetRSSI();
  gettingRssi = false;

  for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
    uint8_t x = LCD_WIDTH * currentStep / stepsCount + exIndex;
    rssiHistory[x] = rssi;
  }
  f += currentStepSize;
  currentStep++;
}

static void step() {
  gettingRssi = true;
  BK4819_TuneTo(f, false); // if true, then bad results O_o
  TaskAdd("Get RSSI", writeRssi, msmTime, false); //->priority = 0;
}

static void startNewScan() {
  currentBandIndex =
      currentBandIndex < bandsCount - 1 ? currentBandIndex + 1 : 0;
  currentBand = &bandsToScan[currentBandIndex];
  currentStepSize = StepFrequencyTable[currentBand->step];

  uint32_t bandwidth = currentBand->bounds.end - currentBand->bounds.start;

  currentStep = 0;
  stepsCount = bandwidth / currentStepSize;
  exLen = ceilDiv(LCD_WIDTH, stepsCount);

  f = currentBand->bounds.start;

  resetRssiHistory();

  BK4819_WriteRegister(0x43, 0b0000000110111100);
  step();
}

static void render() {
  /* const uint16_t rssiMin = 50;
  const uint16_t rssiMax = 130; */
  const uint16_t rssiMin = Min(rssiHistory, LCD_WIDTH);
  const uint16_t rssiMax = Max(rssiHistory, LCD_WIDTH);
  const uint16_t vMin = rssiMin - 2;
  const uint16_t vMax = rssiMax + 20 + (rssiMax - rssiMin) / 2;

  UI_ClearStatus();
  UI_ClearScreen();

  UI_PrintStringSmallest(currentBand->name, 0, 0, true, true);

  char String[16];

  uint32_t fs = currentBand->bounds.start;
  uint32_t fe = currentBand->bounds.end;
  sprintf(String, "%u.%05u", fs / 100000, fs % 100000);
  UI_PrintStringSmallest(String, 0, 49, false, true);

  /* sprintf(String, "\xB1%uk", settings.frequencyChangeStep / 100);
  UI_PrintStringSmallest(String, 52, 49, false, true); */

  sprintf(String, "%u.%05u", fe / 100000, fe % 100000);
  UI_PrintStringSmallest(String, 93, 49, false, true);

  sprintf(String, "%u %u", exLen, stepsCount);
  UI_PrintStringSmallest(String, 42, 49, false, true);

  UI_FSmall(fs);

  for (uint8_t x = 0; x < LCD_WIDTH; ++x) {
    uint8_t yVal = ConvertDomain(rssiHistory[x], vMin, vMax, 0, S_HEIGHT);
    DrawHLine(S_BOTTOM - yVal, S_BOTTOM, x, true);
  }
}

static void addBand(const Band band) { bandsToScan[bandsCount++] = band; }

void SPECTRUM_init(void) {
  resetRssiHistory();
  Band band = {
      .name = "Manual",
      .bounds.start = 43307500,
      .bounds.end = 43627500,
      .step = STEP_25_0kHz,
      .bw = BK4819_FILTER_BW_NARROW,
      .modulation = MOD_FM,
      .squelch = 3,
      .squelchType = SQUELCH_RSSI,
  };
  addBand(band);
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

void SPECTRUM_update(void) {
  if (gettingRssi) {
    return;
  }
  if (f >= currentBand->bounds.end) {
    gRedrawScreen = true;
    return;
  }
  step();
}

void SPECTRUM_render(void) {
  render();
  startNewScan(); // bad practice, but we need to render before
}
