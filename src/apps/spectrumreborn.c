#include "spectrumreborn.h"
#include "../dcs.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../scheduler.h"
#include "../settings.h"
#include "../svc.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"

#define DATA_LEN 128

static const uint16_t U16_MAX = 65535;

// TODO: use as variable
static uint8_t noiseOpenDiff = 14;

static const uint8_t S_HEIGHT = 40;

static const uint8_t SPECTRUM_Y = 16;
static const uint8_t S_BOTTOM = SPECTRUM_Y + S_HEIGHT;

static uint16_t rssiHistory[DATA_LEN] = {0};
static uint16_t noiseHistory[DATA_LEN] = {0};
static bool markers[DATA_LEN] = {0};

static uint8_t x;

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

static bool bandFilled = false;

static uint16_t ceilDiv(uint16_t a, uint16_t b) { return (a + b - 1) / b; }

static void resetRssiHistory() {
  rssiO = U16_MAX;
  noiseO = 0;
  for (uint8_t x = 0; x < DATA_LEN; ++x) {
    rssiHistory[x] = 0;
    noiseHistory[x] = 255;
    markers[x] = false;
  }
}

static Loot msm = {0};

static bool isSquelchOpen() { return msm.rssi >= rssiO && msm.noise <= noiseO; }

static void updateMeasurements() {
  msm.rssi = BK4819_GetRSSI();
  msm.noise = BK4819_GetNoise();
  // UART_printf("%u: Got rssi\n", elapsedMilliseconds);

  if (gIsListening) {
    noiseO -= noiseOpenDiff;
    msm.open = isSquelchOpen();
    noiseO += noiseOpenDiff;
  } else {
    msm.open = isSquelchOpen();
  }

  LOOT_Update(&msm);

  uint8_t ox = x;
  for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
    x = DATA_LEN * currentStep / stepsCount + exIndex;
    if (ox != x) {
      rssiHistory[x] = 0;
      noiseHistory[x] = 255;
      markers[x] = false;
      ox = x;
    }
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

uint32_t lastRender = 0;

static void writeRssi() {
  updateMeasurements();

  RADIO_ToggleRX(msm.open);
  if (msm.open || Now() - lastRender >= 1000) {
    lastRender = Now();
    gRedrawScreen = true;
    return;
  }

  msm.f += currentStepSize;
  currentStep++;
}

static void setF() {
  msm.rssi = 0;
  msm.blacklist = false;
  msm.noise = 255;
  for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
    uint8_t lx = DATA_LEN * currentStep / stepsCount + exIndex;
    noiseHistory[lx] = 255;
    rssiHistory[lx] = 0;
    markers[lx] = false;
  }
  // need to run when task activated, coz of another tasks exists between
  BK4819_TuneTo(msm.f, false);
  /* BK4819_WriteRegister(BK4819_REG_30, 0x200);
  BK4819_WriteRegister(BK4819_REG_30, 0xBFF1); */
  SYSTEM_DelayMs(msmDelay); // (X_X)
  writeRssi();
}

static void step() { setF(); }

static void updateStats() {
  const uint16_t noiseFloor = Std(rssiHistory, x);
  const uint16_t noiseMax = Max(noiseHistory, x);
  rssiO = noiseFloor;
  noiseO = noiseMax - noiseOpenDiff;
}

static void startNewScan() {
  currentStep = 0;
  currentBand = &gCurrentPreset->band;
  currentStepSize = StepFrequencyTable[currentBand->step];

  bandwidth = currentBand->bounds.end - currentBand->bounds.start;

  stepsCount = bandwidth / currentStepSize;
  exLen = ceilDiv(DATA_LEN, stepsCount);

  msm.f = currentBand->bounds.start;

  if (gSettings.activePreset != oldPresetIndex) {
    resetRssiHistory();
    LOOT_Standby();
    rssiO = U16_MAX;
    noiseO = 0;
    RADIO_SetupBandParams(currentBand);
    oldPresetIndex = gSettings.activePreset;
    gRedrawScreen = true;
    bandFilled = false;
  } else {
    bandFilled = true;
  }
}

void SPECTRUM_init(void) {
  SVC_Toggle(SVC_LISTEN, false, 0);
  RADIO_ToggleRX(false);
  RADIO_LoadCurrentVFO();
  resetRssiHistory();
  bandFilled = false;
  newScan = true;

  step();
}

void SPECTRUM_deinit() {
  RADIO_ToggleRX(false);
  SVC_Toggle(SVC_LISTEN, true, gSettings.scanTimeout);
}

bool SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  switch (Key) {
  case KEY_EXIT:
    APPS_exit();
    return true;
  case KEY_UP:
    PRESETS_SelectPresetRelative(true);
    RADIO_SelectPresetSave(gSettings.activePreset);
    newScan = true;
    return true;
  case KEY_DOWN:
    PRESETS_SelectPresetRelative(false);
    RADIO_SelectPresetSave(gSettings.activePreset);
    newScan = true;
    return true;
  case KEY_SIDE1:
    LOOT_BlacklistLast();
    return true;
  case KEY_SIDE2:
    LOOT_GoodKnownLast();
    return true;
  case KEY_F:
    APPS_run(APP_PRESET_CFG);
    return true;
  case KEY_0:
    APPS_run(APP_PRESETS_LIST);
    return true;
  case KEY_STAR:
    APPS_run(APP_LOOT_LIST);
    return true;
  case KEY_5:
    return true;
  case KEY_1:
    IncDec8(&msmDelay, 0, 20, 1);
    resetRssiHistory();
    newScan = true;
    return true;
  case KEY_7:
    IncDec8(&msmDelay, 0, 20, -1);
    resetRssiHistory();
    newScan = true;
    return true;
  case KEY_3:
    IncDec8(&noiseOpenDiff, 2, 40, 1);
    resetRssiHistory();
    newScan = true;
    return true;
  case KEY_9:
    IncDec8(&noiseOpenDiff, 2, 40, -1);
    resetRssiHistory();
    newScan = true;
    return true;
  case KEY_PTT:
    if (gLastActiveLoot) {
      RADIO_TuneToSave(gLastActiveLoot->f);
      APPS_run(APP_STILL);
    }
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

static int RssiMin(uint16_t *array, uint8_t n) {
  uint8_t min = array[0];
  for (uint8_t i = 1; i < n; ++i) {
    if (array[i] == 0) {
      continue;
    }
    if (array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

void SPECTRUM_render(void) {
  UI_ClearScreen();
  STATUSLINE_SetText(currentBand->name);

  UI_DrawTicks(0, DATA_LEN - 1, 56, currentBand);

  const uint8_t xMax = bandFilled ? DATA_LEN - 1 : x;

  const uint16_t rssiMin = RssiMin(rssiHistory, xMax);
  const uint16_t rssiMax = Max(rssiHistory, xMax);
  const uint16_t vMin = rssiMin - 2;
  const uint16_t vMax = rssiMax + 20 + (rssiMax - rssiMin) / 2;

  for (uint8_t xx = 0; xx < xMax; ++xx) {
    uint8_t yVal = ConvertDomain(rssiHistory[xx], vMin, vMax, 0, S_HEIGHT);
    DrawVLine(xx, S_BOTTOM - yVal, yVal, C_FILL);
    if (markers[xx]) {
      DrawVLine(xx, S_BOTTOM + 6, 2, C_FILL);
    }
  }
  DrawHLine(0, S_BOTTOM, DATA_LEN, C_FILL);

  PrintSmallEx(0, SPECTRUM_Y - 3, POS_L, C_FILL, "%ums", msmDelay);
  PrintSmallEx(DATA_LEN - 2, SPECTRUM_Y - 3, POS_R, C_FILL, "SQ %u",
               noiseOpenDiff);
  PrintSmallEx(DATA_LEN - 2, SPECTRUM_Y - 3 + 8, POS_R, C_FILL, "%s",
               modulationTypeOptions[currentBand->modulation]);

  if (gLastActiveLoot) {
    PrintMediumBoldEx(LCD_XCENTER, 16, POS_C, C_FILL, "%u.%05u",
                      gLastActiveLoot->f / 100000, gLastActiveLoot->f % 100000);
    if (gLastActiveLoot->ct != 0xFF) {
      PrintSmallEx(LCD_XCENTER, 16 + 6, POS_C, C_FILL, "CT:%u.%uHz",
                   CTCSS_Options[gLastActiveLoot->ct] / 10,
                   CTCSS_Options[gLastActiveLoot->ct] % 10);
    }
  }

  uint32_t fs = currentBand->bounds.start;
  uint32_t fe = currentBand->bounds.end;

  PrintSmallEx(0, LCD_HEIGHT - 1, POS_L, C_FILL, "%u.%05u", fs / 100000,
               fs % 100000);
  PrintSmallEx(LCD_WIDTH, LCD_HEIGHT - 1, POS_R, C_FILL, "%u.%05u", fe / 100000,
               fe % 100000);
}
