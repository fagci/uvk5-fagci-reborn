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
static const uint8_t SPECTRUM_Y = 8;
static const uint8_t S_BOTTOM = SPECTRUM_Y + S_HEIGHT;

static uint32_t f;
static uint16_t rssiHistory[LCD_WIDTH] = {0};
static bool markers[LCD_WIDTH] = {0};

static Band bandsToScan[32] = {0};
static uint8_t bandsCount = 0;
static uint8_t currentBandIndex = 255;
static Band *currentBand;

static uint32_t currentStepSize;
static uint8_t exLen;
static uint16_t stepsCount;
static uint16_t currentStep;

static uint8_t msmTime = 10;

static bool gettingRssi = false;

static void resetRssiHistory() {
  for (uint8_t x = 0; x < LCD_WIDTH; ++x) {
    rssiHistory[x] = 0;
    markers[x] = false;
  }
}

static uint16_t ceilDiv(uint16_t a, uint16_t b) { return (a + b - 1) / b; }

static void writeRssi() {
  uint16_t rssi = BK4819_GetRSSI();
  // bool open = (BK4819_ReadRegister(BK4819_REG_0C) >> 1) & 1;
  bool open = BK4819_IsSquelchOpen();
  BK4819_ResetRSSI();
  gettingRssi = false;

  for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
    uint8_t x = LCD_WIDTH * currentStep / stepsCount + exIndex;
    if (rssi > rssiHistory[x]) {
      rssiHistory[x] = rssi;
    }
    if (markers[x] == false) {
      markers[x] = open;
    }
  }
  f += currentStepSize;
  currentStep++;
}

static void step() {
  gettingRssi = true;
  BK4819_TuneTo(f, true);
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

  RADIO_SetupBandParams(currentBand);
  BK4819_WriteRegister(0x43, BK4819_FILTER_BW_WIDE);
  // BK4819_WriteRegister(0x43, 0b0000000110111100);
  step();
}

static void DrawTicks() {
  for (uint16_t step = 0; step < stepsCount; step++) {
    uint8_t x = LCD_WIDTH * step / stepsCount;
    uint32_t f = currentBand->bounds.start + step * currentStepSize;
    uint8_t barValue = 0b00000001;
    (f % 10000) < currentStepSize && (barValue |= 0b00000010);
    (f % 50000) < currentStepSize && (barValue |= 0b00000100);
    (f % 100000) < currentStepSize && (barValue |= 0b00011000);

    gFrameBuffer[5][x] |= barValue;
  }

  // center
  if (false) {
    gFrameBuffer[5][62] |= 0x80;
    gFrameBuffer[5][63] |= 0x80;
    gFrameBuffer[5][64] |= 0xff;
    gFrameBuffer[5][65] |= 0x80;
    gFrameBuffer[5][66] |= 0x80;
  } else {
    gFrameBuffer[5][0] |= 0xff;
    gFrameBuffer[5][1] |= 0x80;
    gFrameBuffer[5][2] |= 0x80;
    gFrameBuffer[5][3] |= 0x80;
    gFrameBuffer[5][124] |= 0x80;
    gFrameBuffer[5][125] |= 0x80;
    gFrameBuffer[5][126] |= 0x80;
    gFrameBuffer[5][127] |= 0xff;
  }
}

static void render() {
  const uint16_t rssiMin = Min(rssiHistory, LCD_WIDTH);
  const uint16_t rssiMax = Max(rssiHistory, LCD_WIDTH);
  const uint16_t vMin = rssiMin - 2;
  const uint16_t vMax = rssiMax + 20 + (rssiMax - rssiMin) / 2;

  UI_ClearStatus();
  UI_ClearScreen();

  UI_PrintStringSmallest(currentBand->name, 0, 0, true, true);

  // UI_PrintSmallest(52, 49, "\xB1%uk", settings.frequencyChangeStep / 100);

  UI_FSmall(currentBand->bounds.start);

  UI_PrintSmallest(42, 49, "%u %u", exLen, stepsCount);

  DrawTicks();
  UI_FSmallest(currentBand->bounds.start, 0, 49);
  UI_FSmallest(currentBand->bounds.end, 93, 49);

  for (uint8_t x = 0; x < LCD_WIDTH; ++x) {
    uint8_t yVal = ConvertDomain(rssiHistory[x], vMin, vMax, 0, S_HEIGHT);
    if (markers[x]) {
      PutPixel(x, 16, true);
      PutPixel(x, 17, true);
    }
    DrawHLine(S_BOTTOM - yVal, S_BOTTOM, x, true);
  }
}

static void addBand(const Band band) { bandsToScan[bandsCount++] = band; }

void SPECTRUM_init(void) {
  bandsCount = 0;
  resetRssiHistory();
  addBand((Band){
      .name = "LPD",
      .bounds.start = 43307500,
      .bounds.end = 43477500,
      .step = STEP_25_0kHz,
      .bw = BK4819_FILTER_BW_WIDE,
      .modulation = MOD_FM,
      .squelch = gCurrentVfo.squelch,
      .gainIndex = gCurrentVfo.gainIndex,
      .squelchType = SQUELCH_RSSI,
  });
}

static void setBaseF(uint32_t f) {
  gCurrentVfo.fRX = f;
  RADIO_SaveCurrentVFO();
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
    gFInputCallback = setBaseF;
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
