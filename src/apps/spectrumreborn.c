#include "spectrumreborn.h"
#include "../driver/audio.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../driver/uart.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include "apps.h"
#include "finput.h"
#include <string.h>

#define BLACKLIST_SIZE 32
#define DATA_LEN 64

static const uint16_t U16_MAX = 65535;
static const uint8_t NOISE_OPEN_DIFF = 14;

static const uint8_t S_HEIGHT = 42;
static const uint8_t SPECTRUM_Y = 0;
static const uint8_t S_BOTTOM = SPECTRUM_Y + S_HEIGHT;

static uint16_t rssiHistory[DATA_LEN] = {0};
static uint16_t noiseHistory[DATA_LEN] = {0};
static uint8_t x;
static bool markers[DATA_LEN] = {0};

static Peak peaks[16] = {0};
static uint8_t peaksCount = 255;

static Band bandsToScan[32] = {0};
static uint8_t bandsCount = 0;
static uint8_t currentBandIndex = 255;
static Band *currentBand;

static uint32_t currentStepSize;
static uint8_t exLen;
static uint16_t stepsCount;
static uint16_t currentStep;
static uint32_t bandwidth;

static bool newScan = true;

static uint16_t rssiO = U16_MAX;
static uint16_t noiseO = 0;

static uint8_t msmDelay = 5;

static uint16_t ceilDiv(uint16_t a, uint16_t b) { return (a + b - 1) / b; }

static void resetRssiHistory() {
  for (uint8_t x = 0; x < DATA_LEN; ++x) {
    rssiHistory[x] = 0;
    noiseHistory[x] = 0;
    markers[x] = false;
  }
}
static void addBand(const Band band) { bandsToScan[bandsCount++] = band; }

static Peak *getPeak(uint32_t f) {
  for (uint8_t i = 0; i < peaksCount; ++i) {
    if ((&peaks[i])->f == f) {
      return &peaks[i];
    }
  }
  return NULL;
}
static Peak *addPeak(uint32_t f) {
  if (peaksCount < 16) {
    peaks[peaksCount] = (Peak){
        .f = f,
        .firstTime = elapsedMilliseconds,
        .lastTimeCheck = elapsedMilliseconds,
        .lastTimeOpen = elapsedMilliseconds,
        .duration = 0,
        .rssi = 0,
        .noise = U16_MAX,
        .open = true, // as we add it when open
    };
    return &peaks[peaksCount++];
  }
  return NULL;
}

static Peak msm = {0};

static bool isSquelchOpen() { return msm.rssi >= rssiO && msm.noise <= noiseO; }

static void updateMeasurements() {
  msm.rssi = BK4819_GetRSSI();
  msm.noise = BK4819_GetNoise();
  UART_printf("%u: Got rssi\n", elapsedMilliseconds);

  msm.open = isSquelchOpen();

  Peak *peak = getPeak(msm.f);

  if (peak == NULL && msm.open) {
    peak = addPeak(msm.f);
  }

  if (peak != NULL) {
    peak->noise = msm.noise;
    peak->rssi = msm.rssi;

    if (peak->open) {
      peak->duration += elapsedMilliseconds - peak->lastTimeCheck;
    }
    if (msm.open) {
      peak->lastTimeOpen = elapsedMilliseconds;
    }
    peak->lastTimeCheck = elapsedMilliseconds;
    peak->open = msm.open;
  }

  if (exLen) {
    for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
      x = DATA_LEN * currentStep / stepsCount + exIndex;
      rssiHistory[x] = msm.rssi;
      noiseHistory[x] = msm.noise;
      markers[x] = msm.open;
    }
  } else {
    x = DATA_LEN * currentStep / stepsCount;
    if (msm.rssi > rssiHistory[x]) {
      rssiHistory[x] = msm.rssi;
    }
    if (msm.noise < noiseHistory[x]) {
      noiseHistory[x] = msm.noise;
    }
    if (markers[x] == false && msm.open) {
      markers[x] = true;
    }
  }
}

static void writeRssi() {
  updateMeasurements();

  RADIO_ToggleRX(msm.open);
  if (msm.open) {
    gRedrawScreen = true;
    return;
  }

  msm.f += currentStepSize;
  currentStep++;
}

static void step() {
  msm.rssi = 0;
  msm.noise = U16_MAX;
  for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
    uint8_t lx = DATA_LEN * currentStep / stepsCount + exIndex;
    noiseHistory[lx] = U16_MAX;
    rssiHistory[lx] = 0;
    markers[lx] = false;
  }

  BK4819_SetFrequency(msm.f);
  BK4819_WriteRegister(BK4819_REG_30, 0x0);
  BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);

  TaskAdd("Get RSSI", writeRssi, msmDelay, false)->priority = 0;
}

static void startNewScan() {
  uint8_t oldBandIndex = currentBandIndex;
  currentBandIndex =
      currentBandIndex < bandsCount - 1 ? currentBandIndex + 1 : 0;
  currentBand = &bandsToScan[currentBandIndex];
  currentStepSize = StepFrequencyTable[currentBand->step];

  bandwidth = currentBand->bounds.end - currentBand->bounds.start;

  currentStep = 0;
  stepsCount = bandwidth / currentStepSize;
  exLen = ceilDiv(DATA_LEN, stepsCount);

  msm.f = currentBand->bounds.start;

  if (currentBandIndex != oldBandIndex) {
    resetRssiHistory();
    RADIO_SetupBandParams(&bandsToScan[0]);
    gRedrawStatus = true;
  }
}

static void DrawTicks() {
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
    gFrameBuffer[5][DATA_LEN - 2] |= 0x80;
    gFrameBuffer[5][DATA_LEN - 1] |= 0xff;
  }

  if (bandwidth > 600000) {
    for (uint16_t step = 0; step < stepsCount; step++) {
      uint8_t x = DATA_LEN * step / stepsCount;
      uint32_t f = currentBand->bounds.start + step * currentStepSize;
      (f % 500000) < currentStepSize && (gFrameBuffer[5][x] |= 0b00110000);
    }
    return;
  }

  for (uint16_t step = 0; step < stepsCount; step++) {
    uint8_t x = DATA_LEN * step / stepsCount;
    uint32_t f = currentBand->bounds.start + step * currentStepSize;
    (f % 10000) < currentStepSize && (gFrameBuffer[5][x] |= 0b00001000);
    (f % 50000) < currentStepSize && (gFrameBuffer[5][x] |= 0b00011000);
    (f % 100000) < currentStepSize && (gFrameBuffer[5][x] |= 0b01110000);
  }
}

void SPECTRUM_init(void) {
  bandsCount = 0;
  newScan = true;

  resetRssiHistory();

  /* addBand((Band){
      .name = "LPD",
      .bounds.start = 43307500,
      .bounds.end = 43477500,
      .step = STEP_25_0kHz,
      .bw = BK4819_FILTER_BW_WIDE,
      .modulation = MOD_FM,
      .gainIndex = gCurrentVfo.gainIndex,
  }); */
  /* addBand((Band){
      .name = "Avia",
      .bounds.start = 11800000,
      .bounds.end = 13000000,
      .step = STEP_25_0kHz,
      .bw = BK4819_FILTER_BW_WIDE,
      .modulation = MOD_AM,
      .gainIndex = gCurrentVfo.gainIndex,
  }); */
  /* addBand((Band){
      .name = "JD",
      .bounds.start = 15171250,
      .bounds.end = 15401250,
      .step = STEP_25_0kHz,
      .bw = BK4819_FILTER_BW_WIDE,
      .modulation = MOD_FM,
      .gainIndex = gCurrentVfo.gainIndex,
  }); */
  addBand((Band){
      .name = "MED",
      .bounds.start = 40605000,
      .bounds.end = 40605000 + 2500 * 32,
      .step = STEP_25_0kHz,
      .bw = BK4819_FILTER_BW_WIDE,
      .modulation = MOD_FM,
      .squelch = 0,
      .squelchType = SQUELCH_RSSI_NOISE,
      .gainIndex = gCurrentVfo.gainIndex,
  });
  /* addBand((Band){
      .name = "TEST",
      .bounds.start = 45200000,
      .bounds.end = 45280000,
      .step = STEP_25_0kHz,
      .bw = BK4819_FILTER_BW_WIDE,
      .modulation = MOD_FM,
      .gainIndex = gCurrentVfo.gainIndex,
      .squelchType = SQUELCH_RSSI_NOISE,
      .squelch = 0,
  }); */
  step();
}

static void updateStats() {
  const uint16_t noiseFloor = Std(rssiHistory, x);
  const uint16_t noiseMax = Max(noiseHistory, x);
  rssiO = noiseFloor;
  noiseO = noiseMax - NOISE_OPEN_DIFF;
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
    return true;
  case KEY_3:
    msmDelay++;
    return true;
  case KEY_9:
    msmDelay--;
    return true;
  case KEY_2:
    return true;
  case KEY_8:
    return true;
  default:
    break;
  }
  return false;
}

bool updateListen() {
  msm.rssi = BK4819_GetRSSI();
  msm.noise = BK4819_GetNoise();
  noiseO -= NOISE_OPEN_DIFF;
  bool open = isSquelchOpen();
  noiseO += NOISE_OPEN_DIFF;
  gRedrawScreen = true;
  return open;
}

void SPECTRUM_update(void) {
  if (msm.rssi == 0) {
    return;
  }
  UART_printf("Spectrum update pass\n");
  if (gIsListening) {
    if (!updateListen()) {
      gRedrawScreen = true;
      RADIO_ToggleRX(false);
    }
    return;
  }
  if (newScan) {
    newScan = false;
    startNewScan();
  }
  if (msm.f >= currentBand->bounds.end) {
    updateStats();
    gRedrawScreen = true;
    newScan = true;
    return;
  }

  step();
}

void SPECTRUM_render(void) {
  const uint16_t rssiMin = Min(rssiHistory, x);
  const uint16_t rssiMax = Max(rssiHistory, x);
  const uint16_t vMin = rssiMin - 2;
  const uint16_t vMax = rssiMax + 20 + (rssiMax - rssiMin) / 2;

  UI_ClearStatus();
  UI_ClearScreen();

  UI_PrintStringSmallest(currentBand->name, 0, 0, true, true);

  DrawTicks();
  UI_FSmallest(currentBand->bounds.start, 0, 49);
  UI_FSmallest(currentBand->bounds.end, 93, 49);

  for (uint8_t xx = 0; xx < DATA_LEN; ++xx) {
    uint8_t yVal = ConvertDomain(rssiHistory[xx], vMin, vMax, 0, S_HEIGHT);
    DrawHLine(S_BOTTOM - yVal, S_BOTTOM, xx, true);
    if (markers[xx]) {
      PutPixel(xx, 46, true);
      PutPixel(xx, 47, true);
    }
  }

  DrawHLine(0, S_BOTTOM, DATA_LEN - 1, true);

  if (peaksCount != 255) {
    Sort(peaks, peaksCount, SortByLastOpenTime);
    for (uint8_t i = 0; i < Clamp(peaksCount, 0, 8); i++) {
      Peak *p = &peaks[i];
      UI_PrintSmallest(DATA_LEN + 1, i * 6, "%c%u.%04u %us",
                       p->open ? '>' : ' ', p->f / 100000, p->f / 10 % 10000,
                       p->duration / 1000);
    }
  }
}
