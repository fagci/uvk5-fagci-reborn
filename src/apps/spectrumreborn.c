#include "spectrumreborn.h"
#include "../driver/audio.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../driver/uart.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../scheduler.h"
#include "../settings.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"
#include <string.h>

#define BLACKLIST_SIZE 32
#define DATA_LEN 64

static const uint16_t U16_MAX = 65535;
static const uint8_t NOISE_OPEN_DIFF = 14;

static const uint8_t S_HEIGHT = 39;

static const uint8_t SPECTRUM_Y = 8;
static const uint8_t S_BOTTOM = SPECTRUM_Y + S_HEIGHT;

static uint16_t rssiHistory[DATA_LEN] = {0};
static uint16_t noiseHistory[DATA_LEN] = {0};
static uint8_t x;
static bool markers[DATA_LEN] = {0};

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

static uint16_t oldPresetIndex = 255;

static uint16_t ceilDiv(uint16_t a, uint16_t b) { return (a + b - 1) / b; }

static void resetRssiHistory() {
  for (uint8_t x = 0; x < DATA_LEN; ++x) {
    rssiHistory[x] = 0;
    noiseHistory[x] = 0;
    markers[x] = false;
  }
}

static Loot msm = {0};

static bool isSquelchOpen() { return msm.rssi >= rssiO && msm.noise <= noiseO; }

static void updateMeasurements() {
  msm.rssi = BK4819_GetRSSI();
  msm.noise = BK4819_GetNoise();
  UART_printf("%u: Got rssi\n", elapsedMilliseconds);

  if (gIsListening) {
    noiseO -= NOISE_OPEN_DIFF;
    msm.open = isSquelchOpen();
    noiseO += NOISE_OPEN_DIFF;
  } else {
    msm.open = isSquelchOpen();
  }

  LOOT_Update(&msm);

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
  msm.blacklist = false;
  msm.noise = U16_MAX;
  for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
    uint8_t lx = DATA_LEN * currentStep / stepsCount + exIndex;
    noiseHistory[lx] = U16_MAX;
    rssiHistory[lx] = 0;
    markers[lx] = false;
  }

  BK4819_SetFrequency(msm.f);
  BK4819_WriteRegister(BK4819_REG_30, 0x200);
  BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);

  TaskAdd("Get RSSI", writeRssi, msmDelay, false)->priority = 0;
}

static void startNewScan() {
  currentStep = 0;
  currentBand = &PRESETS_Item(gSettings.activePreset)->band;
  currentStepSize = StepFrequencyTable[currentBand->step];

  bandwidth = currentBand->bounds.end - currentBand->bounds.start;

  stepsCount = bandwidth / currentStepSize;
  exLen = ceilDiv(DATA_LEN, stepsCount);

  msm.f = currentBand->bounds.start;

  if (gSettings.activePreset != oldPresetIndex) {
    resetRssiHistory();
    LOOT_Clear();
    LOOT_Standby();
    rssiO = U16_MAX;
    noiseO = 0;
    RADIO_SetupBandParams(currentBand);
    oldPresetIndex = gSettings.activePreset;
    gRedrawScreen = true;
  }
}

void SPECTRUM_init(void) {
  newScan = true;

  resetRssiHistory();
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
  case KEY_UP:
    PRESETS_SelectPresetRelative(true);
    newScan = true;
    return true;
  case KEY_DOWN:
    PRESETS_SelectPresetRelative(false);
    newScan = true;
    return true;
  case KEY_SIDE1:
    LOOT_BlacklistLast();
    return true;
  case KEY_F:
    APPS_run(APP_PRESET_CFG);
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

void SPECTRUM_update(void) {
  if (msm.rssi == 0) {
    return;
  }
  if (newScan || gSettings.activePreset != oldPresetIndex) {
    newScan = false;
    startNewScan();
  }
  UART_printf("Spectrum update pass\n");
  if (gIsListening) {
    updateMeasurements();
    gRedrawScreen = true;
    if (!msm.open) {
      RADIO_ToggleRX(false);
    }
    return;
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

  PrintSmall(0, 5, currentBand->name);

  UI_DrawTicks(0, DATA_LEN, 6, currentBand, false);
  UI_FSmallest(currentBand->bounds.start, 0, SPECTRUM_Y + S_HEIGHT + 8 + 6);
  UI_FSmallest(currentBand->bounds.end, 93, SPECTRUM_Y + S_HEIGHT + 8 + 6);

  for (uint8_t xx = 0; xx < DATA_LEN; ++xx) {
    uint8_t yVal = ConvertDomain(rssiHistory[xx], vMin, vMax, 0, S_HEIGHT);
    DrawVLine(xx, S_BOTTOM - yVal, yVal, true);
    if (markers[xx]) {
      DrawVLine(xx, SPECTRUM_Y + S_HEIGHT + 6, 2, true);
    }
  }

  DrawVLine(DATA_LEN - 1, SPECTRUM_Y, S_BOTTOM, true);

  LOOT_Sort(LOOT_SortByLastOpenTime);

  for (uint8_t i = 0; i < Clamp(LOOT_Size(), 0, 8); i++) {
    Loot *p = LOOT_Item(i);
    PrintSmall(DATA_LEN + 1, i * 6 + 5 + 8, "%c",
               p->open ? '>' : (p->blacklist ? 'X' : ' '));
    PrintSmall(DATA_LEN + 1 + 8, i * 6 + 5 + 8, "%u.%04u %u", p->f / 100000,
               p->f / 10 % 10000, p->ct);
  }
}
