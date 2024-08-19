#include "spectrumreborn.h"
#include "../dcs.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../driver/systick.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../scheduler.h"
#include "../settings.h"
#include "../svc.h"
#include "../svc_scan.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/spectrum.h"
#include "../ui/statusline.h"
#include "apps.h"

#define DATA_LEN 128

static const uint16_t U16_MAX = 65535;

static uint8_t noiseOpenDiff = 14;

static const uint8_t S_HEIGHT = 40;

static const uint8_t SPECTRUM_Y = 16;

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
static Loot msm = {0};

static uint16_t oldPresetIndex = 255;

static bool bandFilled = false;

static uint32_t timeout = 0;
static bool lastListenState = false;

static const uint16_t BK_RST_HARD = 0x200;
static const uint16_t BK_RST_SOFT = 0xBFF1 & ~BK4819_REG_30_ENABLE_VCO_CALIB;

static bool softResetRssi = true;
static uint16_t resetBkVal = BK_RST_SOFT;

static bool isSquelchOpen() { return msm.rssi >= rssiO && msm.noise <= noiseO; }

static uint16_t ceilDiv(uint16_t a, uint16_t b) { return (a + b - 1) / b; }

static void updateMeasurements() {
  if (!gIsListening) {
    BK4819_WriteRegister(BK4819_REG_38, msm.f & 0xFFFF);
    BK4819_WriteRegister(BK4819_REG_39, (msm.f >> 16) & 0xFFFF);
    BK4819_WriteRegister(BK4819_REG_30, resetBkVal);
    BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);
    SYSTICK_DelayUs(msmDelay * 1000); // (X_X)
  }
  msm.blacklist = false;
  msm.rssi = BK4819_GetRSSI();
  msm.noise = BK4819_GetNoise();

  if (gIsListening) {
    noiseO -= noiseOpenDiff;
    msm.open = isSquelchOpen();
    noiseO += noiseOpenDiff;
  } else {
    msm.open = isSquelchOpen();
  }

  if (lastListenState != msm.open) {
    lastListenState = msm.open;
    SetTimeout(&timeout, msm.open ? SCAN_TIMEOUTS[gSettings.sqOpenedTimeout]
                                  : SCAN_TIMEOUTS[gSettings.sqClosedTimeout]);
  }

  if (timeout && CheckTimeout(&timeout)) {
    msm.open = false;
  }
  SP_AddPoint(&msm);
  LOOT_Update(&msm);
  if (bandFilled) {
    RADIO_ToggleRX(msm.open);
  }
}

uint32_t lastRender = 0;

static void step() { updateMeasurements(); }

static void updateStats() {
  const uint16_t noiseFloor = SP_GetNoiseFloor();
  const uint16_t noiseMax = SP_GetNoiseMax();
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
    oldPresetIndex = gSettings.activePreset;
    rssiO = U16_MAX;
    noiseO = 0;
    radio->radio = RADIO_BK4819;
    LOOT_Standby();
    RADIO_SetupBandParams();
    SP_Init(PRESETS_GetSteps(gCurrentPreset), LCD_WIDTH);
    gRedrawScreen = true;
    bandFilled = false;
  } else {
    SP_Begin();
    bandFilled = true;
  }
}

void SPECTRUM_init(void) {
  SVC_Toggle(SVC_LISTEN, false, 0);
  RADIO_LoadCurrentVFO();
  radio->radio = RADIO_BK4819;
  RADIO_SetupBandParams();
  newScan = true;
  timeout = 0;
  oldPresetIndex = 0;

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
  case KEY_4:
    softResetRssi = !softResetRssi;
    resetBkVal = softResetRssi ? BK_RST_SOFT : BK_RST_HARD;
    SP_ResetHistory();
    newScan = true;
    return true;
  case KEY_1:
    IncDec8(&msmDelay, 0, 20, 1);
    SP_ResetHistory();
    newScan = true;
    return true;
  case KEY_7:
    IncDec8(&msmDelay, 0, 20, -1);
    SP_ResetHistory();
    newScan = true;
    return true;
  case KEY_3:
    IncDec8(&noiseOpenDiff, 2, 40, 1);
    return true;
  case KEY_9:
    IncDec8(&noiseOpenDiff, 2, 40, -1);
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
  if (Now() - lastRender >= 500) {
    lastRender = Now();
    gRedrawScreen = true;
  }
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
    lastRender = Now();
  }
  if (gIsListening) {
    return;
  }
  if (msm.f >= currentBand->bounds.end) {
    updateStats();
    gRedrawScreen = true; // FIXME: first msm high??!
    lastRender = Now();
    newScan = true;
    return;
  }

  msm.f += currentStepSize;
  if (gSettings.skipGarbageFrequencies && (msm.f % 1300000 == 0)) {
    msm.f += currentStepSize;
  }
  currentStep++;
  SP_Next();
  step();
}

void SPECTRUM_render(void) {
  UI_ClearScreen();
  STATUSLINE_SetText(currentBand->name);

  SP_Render(gCurrentPreset, 0, SPECTRUM_Y, S_HEIGHT);

  PrintSmallEx(0, SPECTRUM_Y - 3, POS_L, C_FILL, "%ums", msmDelay);
  PrintSmallEx(0, SPECTRUM_Y - 3 + 6, POS_L, C_FILL, "%s",
               softResetRssi ? "Soft" : "Hard");
  PrintSmallEx(DATA_LEN - 2, SPECTRUM_Y - 3, POS_R, C_FILL, "SQ %u",
               noiseOpenDiff);
  PrintSmallEx(DATA_LEN - 2, SPECTRUM_Y - 3 + 8, POS_R, C_FILL, "%s",
               modulationTypeOptions[currentBand->modulation]);

  if (gLastActiveLoot) {
    PrintMediumBoldEx(LCD_XCENTER, 16, POS_C, C_FILL, "%u.%05u",
                      gLastActiveLoot->f / 100000, gLastActiveLoot->f % 100000);
    if (gLastActiveLoot->ct != 0xFF) {
      PrintSmallEx(LCD_XCENTER, 16 + 6, POS_C, C_FILL, "CT %u.%u",
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
