#include "spectrumreborn.h"
#include "../driver/audio.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../helper/measurements.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include "apps.h"
#include "finput.h"
#include <string.h>

#define BLACKLIST_SIZE 32
#define DATA_LEN 64

static const uint8_t S_HEIGHT = 32;
static const uint8_t SPECTRUM_Y = 8;
static const uint8_t S_BOTTOM = SPECTRUM_Y + S_HEIGHT;

typedef struct {
  uint32_t f;
  uint32_t rssi;
  uint32_t noise;
} Peak;

static uint32_t f;
static uint16_t rssiHistory[DATA_LEN] = {0};
static uint16_t noiseHistory[DATA_LEN] = {0};
static uint16_t glitchHistory[DATA_LEN] = {0};
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

static bool gettingRssi = false;

static bool newScan = true;

static uint16_t listenT = 0;

#define U16_MAX 65535

static uint16_t rssiO = U16_MAX;
static uint16_t noiseO = U16_MAX;
static uint16_t glitchO = U16_MAX;

static uint16_t ceilDiv(uint16_t a, uint16_t b) { return (a + b - 1) / b; }

static void resetRssiHistory() {
  for (uint8_t x = 0; x < DATA_LEN; ++x) {
    rssiHistory[x] = 0;
    noiseHistory[x] = 0;
    glitchHistory[x] = 0;
    markers[x] = false;
  }
}

#include "../driver/uart.h"
static void addBand(const Band band) { bandsToScan[bandsCount++] = band; }
static void addPeak(const Peak peak) {
  for (uint8_t i = 0; i < peaksCount; i++) {
    if (peaks[i].f == peak.f) {
      peaks[i].noise = peak.noise;
      peaks[i].rssi = peak.rssi;
      return;
    }
  }
  if (peaksCount < 16) {
    peaks[peaksCount++] = peak;
  }
}

static void writeRssi() {
  uint16_t rssi = BK4819_GetRSSI();
  uint16_t noise = BK4819_GetNoise();
  uint16_t glitch = BK4819_GetGlitch();
  // bool open = BK4819_IsSquelchOpen();
  bool open = rssi >= rssiO && noise <= noiseO && glitch <= glitchO;
  gettingRssi = false;

  if (rssi >= rssiO && noise <= noiseO) {
    addPeak((Peak){.f = f, .rssi = rssi, .noise = noise});
  }

  for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
    x = DATA_LEN * currentStep / stepsCount + exIndex;
    if (rssi > rssiHistory[x]) {
      rssiHistory[x] = rssi;
    }
    if (noise > noiseHistory[x]) {
      noiseHistory[x] = noise;
    }
    if (glitch > glitchHistory[x]) {
      glitchHistory[x] = glitch;
    }
    if (markers[x] == false) {
      markers[x] = open;
    }
  }

  /* RADIO_ToggleRX(open);
  if (open) {
    listenT = 1000;
    gRedrawScreen = true;
    return;
  } */

  f += currentStepSize;
  currentStep++;
}

static void SetF(uint32_t f) {
  BK4819_SetFrequency(f);
  BK4819_WriteRegister(BK4819_REG_30, 0xBFF1 & ~BK4819_REG_30_ENABLE_VCO_CALIB);
  BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);
  SYSTEM_DelayMs(1);
}

static void step() {
  gettingRssi = true;
  SetF(f);
  TaskAdd("Get RSSI", writeRssi, 1, false); // ->priority = 0;
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

  f = currentBand->bounds.start;

  resetRssiHistory();
  if (currentBandIndex != oldBandIndex) {
    RADIO_SetupBandParams(&bandsToScan[0]);
  }
  SetF(f - StepFrequencyTable[currentBand->step]); // to make first measurement
                                                   // better
  step();
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
      (f % 500000) < currentStepSize && (gFrameBuffer[5][x] |= 0b00011111);
    }
    return;
  }

  for (uint16_t step = 0; step < stepsCount; step++) {
    uint8_t x = DATA_LEN * step / stepsCount;
    uint32_t f = currentBand->bounds.start + step * currentStepSize;
    uint8_t barValue = 0b00000001;
    (f % 10000) < currentStepSize && (barValue |= 0b00000010);
    (f % 50000) < currentStepSize && (barValue |= 0b00000100);
    (f % 100000) < currentStepSize && (barValue |= 0b00011000);

    gFrameBuffer[5][x] |= barValue;
  }
}

void SPECTRUM_init(void) {
  bandsCount = 0;
  newScan = true;

  BK4819_WriteRegister(0x2B, 0); // various filters
  BK4819_WriteRegister(0x73,
                       BK4819_ReadRegister(0x73) | (1 << 4)); // AFC disable
  BK4819_WriteRegister(BK4819_REG_3F, 0);                     // interrupts

  /* BK4819_WriteRegister(
      0x43,
      0 | 1 << 2        // gain after FM demod = 6dB
          | 0b10 << 4   // BW mode selection:00: 12.5k 01: 6.25k 10: 25k/20k
          | 0b000 << 6  // LPF BW
          | 0b000 << 9  // RF BW weak
          | 0b000 << 12 // RF BW
  ); */

  resetRssiHistory();
  addBand((Band){
      .name = "LPD",
      .bounds.start = 43307500,
      .bounds.end = 43477500,
      .step = STEP_25_0kHz,
      .bw = BK4819_FILTER_BW_WIDE,
      .modulation = MOD_FM,
      .squelch = 3, // gCurrentVfo.squelch,
      .gainIndex = gCurrentVfo.gainIndex,
      .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
  });
  /* addBand((Band){
      .name = "MED",
      .bounds.start = 40605000,
      .bounds.end = 40605000 + 2500 * 64 * 32,
      .step = STEP_25_0kHz,
      .bw = BK4819_FILTER_BW_WIDE,
      .modulation = MOD_FM,
      .squelch = 3, // gCurrentVfo.squelch,
      .gainIndex = gCurrentVfo.gainIndex,
      .squelchType = SQUELCH_RSSI,
  }); */
  /* addBand((Band){
      .name = "TEST",
      .bounds.start = 45207500,
      .bounds.end = 45277500,
      .step = STEP_25_0kHz,
      .bw = BK4819_FILTER_BW_WIDE,
      .modulation = MOD_FM,
      .squelch = 3, // gCurrentVfo.squelch,
      .gainIndex = gCurrentVfo.gainIndex,
      .squelchType = SQUELCH_RSSI,
  }); */
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
  case KEY_3:
    return true;
  case KEY_9:
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

void SPECTRUM_update(void) {
  if (listenT) {
    if (--listenT == 0 || !BK4819_IsSquelchOpen()) {
      listenT = 0;
    } else {
      return;
    }
  }
  if (gettingRssi) {
    return;
  }
  if (newScan) {
    newScan = false;
    startNewScan();
  }
  if (f >= currentBand->bounds.end) {
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

  const uint16_t noiseFloor = Std(rssiHistory, x);
  const uint16_t noiseMax = Max(noiseHistory, x);
  const uint16_t glitchMax = Max(glitchHistory, x);

  rssiO = noiseFloor + 1;
  noiseO = noiseMax - 1;
  glitchO = glitchMax - 1;

  // BK4819_SetupSquelch(rssiO, rssiC, noiseO, noiseC, glitchO, glitchC);

  UI_ClearStatus();
  UI_ClearScreen();

  UI_PrintStringSmallest(currentBand->name, 0, 0, true, true);

  // UI_PrintSmallest(52, 49, "\xB1%uk", settings.frequencyChangeStep / 100);

  UI_FSmall(currentBand->bounds.start);

  DrawTicks();
  UI_FSmallest(currentBand->bounds.start, 0, 49);
  UI_FSmallest(currentBand->bounds.end, 93, 49);

  for (uint8_t xx = 0; xx < x; ++xx) {
    uint8_t yVal = ConvertDomain(rssiHistory[xx], vMin, vMax, 0, S_HEIGHT);
    DrawHLine(S_BOTTOM - yVal, S_BOTTOM, xx, true);
    if (markers[xx]) {
      PutPixel(xx, 46, true);
      PutPixel(xx, 47, true);
      /* if (xx > 0)
        PutPixel(xx - 1, 47, true);
      if (xx < LCD_WIDTH - 1)
        PutPixel(xx + 1, 47, true); */
    }
  }

  for (uint8_t x = 0; x < DATA_LEN; x++) {
    PutPixel(x, S_BOTTOM - ConvertDomain(rssiO, vMin, vMax, 0, S_HEIGHT), 2);
  }

  DrawHLine(16, S_BOTTOM, DATA_LEN - 1, true);

  for (uint8_t i = 0; i < 5; i++) {
    Peak p = peaks[i];
    UI_PrintSmallest(65, i * 6 + 16, "%u.%04u %u %u", p.f / 100000,
                     p.f / 10 % 10000, p.rssi, p.noise);
  }
}
