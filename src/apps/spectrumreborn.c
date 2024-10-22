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
#include "../svc_render.h"
#include "../svc_scan.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/spectrum.h"
#include "../ui/statusline.h"
#include "apps.h"

static const uint16_t U16_MAX = 65535;

static uint8_t noiseOpenDiff = 14;

static const uint8_t SPECTRUM_Y = 16;
static const uint8_t SPECTRUM_H = 40;

static Band *currentBand;

static uint32_t currentStepSize;

static bool newScan = true;

static uint16_t rssiO = U16_MAX;
static uint16_t noiseO = 0;

static uint8_t msmDelay = 5;
static Loot msm = {0};

static uint16_t oldPresetIndex = 255;

static const uint16_t BK_RST_HARD = 0x0200;
static const uint16_t BK_RST_SOFT = 0xBFF1 & ~BK4819_REG_30_ENABLE_VCO_CALIB;

static const uint16_t RESET_METHODS[] = {BK_RST_HARD, BK_RST_SOFT};
static const char *RESET_METHOD_NAMES[] = {"Hard", "Soft"};

static uint8_t rssiResetMethod = 0;
static uint16_t resetBkVal = BK_RST_HARD;

static bool isSquelchOpen() { return msm.rssi >= rssiO && msm.noise <= noiseO; }

static void updateStats() {
  const uint16_t noiseFloor = SP_GetNoiseFloor();
  const uint16_t noiseMax = SP_GetNoiseMax();
  rssiO = noiseFloor;
  noiseO = noiseMax - noiseOpenDiff;
}

static void updateMsm() {
  if (!gIsListening) {
    BK4819_SetFrequency(msm.f);
    BK4819_WriteRegister(BK4819_REG_30, resetBkVal);
    BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);
    SYSTEM_DelayMs(msmDelay); // (X_X)
  }
  msm.rssi = BK4819_GetRSSI();
  msm.noise = BK4819_GetNoise();
  msm.blacklist = false;

  if (gIsListening) {
    noiseO -= noiseOpenDiff;
    msm.open = isSquelchOpen();
    noiseO += noiseOpenDiff;
  } else {
    msm.open = isSquelchOpen();
  }

  SP_AddPoint(&msm);
  LOOT_Update(&msm);
  RADIO_ToggleRX(msm.open);
}

static void scanFn(bool forward) {
  RADIO_ToggleRX(false);
  radio->rx.f = msm.f;
  if (RADIO_NextPresetFreqXBandEx(true, false, false)) {
    if (noiseO > 0) {
      // next band scan
      msm.f = radio->rx.f;
    } else {
      // rewind
      updateStats();
      PRESET_Select(oldPresetIndex);
    }
    newScan = true;
  } else {
    msm.f = radio->rx.f;
    SP_Next();
  }
}

static void init() {
  newScan = true;
  oldPresetIndex = 0;
  rssiO = U16_MAX;
  noiseO = 0;

  radio->radio = RADIO_BK4819;

  LOOT_Standby();
  RADIO_SetupBandParams();

  SP_Init(PRESETS_GetSteps(gCurrentPreset), LCD_WIDTH);
  updateMsm();
}

static void startNewScan() {
  currentBand = &gCurrentPreset->band;
  currentStepSize = PRESETS_GetStepSize(gCurrentPreset);

  msm.f = currentBand->bounds.start;

  if (gSettings.activePreset != oldPresetIndex) {
    init();
    oldPresetIndex = gSettings.activePreset;
  } else {
    SP_Begin();
  }
}

void SPECTRUM_init(void) {
  SVC_Toggle(SVC_LISTEN, false, 0);
  RADIO_LoadCurrentVFO();
  init();
  // SVC_Toggle(SVC_SCAN, true, msmDelay);
}

void SPECTRUM_deinit() {
  SVC_Toggle(SVC_SCAN, false, msmDelay);
  BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);
  RADIO_ToggleRX(false);
  SVC_Toggle(SVC_LISTEN, true, msmDelay);
  RADIO_SetupBandParams();
}

bool SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (Key) {
    case KEY_UP:
      PRESETS_SelectPresetRelative(true);
      RADIO_SelectPresetSave(gSettings.activePreset);
      // newScan = true;
      startNewScan();
      return true;
    case KEY_DOWN:
      PRESETS_SelectPresetRelative(false);
      RADIO_SelectPresetSave(gSettings.activePreset);
      // newScan = true;
      startNewScan();
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
    default:
      break;
    }
  }

  // long held
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (Key) {
    case KEY_SIDE1:
      if (gLastActiveLoot) {
        RADIO_TuneToSave(gLastActiveLoot->f);
        APPS_run(APP_ANALYZER);
      }
      return true;
    default:
      break;
    }
  }

  // Simple keypress
  if (!bKeyPressed && !bKeyHeld) {
    switch (Key) {
    case KEY_EXIT:
      APPS_exit();
      return true;
    case KEY_SIDE1:
      LOOT_BlacklistLast();
      RADIO_NextFreqNoClicks(true);
      return true;
    case KEY_SIDE2:
      LOOT_GoodKnownLast();
      RADIO_NextFreqNoClicks(true);
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
      rssiResetMethod = !rssiResetMethod;
      resetBkVal = RESET_METHODS[rssiResetMethod];
      SP_ResetHistory();
      newScan = true;
      return true;
    case KEY_PTT:
      if (gLastActiveLoot) {
        RADIO_TuneToSave(gLastActiveLoot->f);
        APPS_run(APP_VFOPRO);
      }
      return true;
    default:
      break;
    }
  }
  return false;
}

void SPECTRUM_update(void) {
  if (Now() - gLastRender >= 500) {
    gRedrawScreen = true;
  }
  if (newScan) {
    newScan = false;
    SVC_Toggle(SVC_SCAN, false, msmDelay);
    startNewScan();
    gScanFn = scanFn;
    SVC_Toggle(SVC_SCAN, true, msmDelay);
  }
  updateMsm();
}

void SPECTRUM_render(void) {
  UI_ClearScreen();
  STATUSLINE_SetText(currentBand->name);

  SP_Render(gCurrentPreset, 0, SPECTRUM_Y, SPECTRUM_H);
  UI_DrawSpectrumElements(SPECTRUM_Y, msmDelay, noiseOpenDiff, currentBand);

  // todo: ct+cd
  if (gLastActiveLoot && gLastActiveLoot->ct != 0xFF) {
    PrintSmallEx(LCD_XCENTER, 16 + 6, POS_C, C_FILL, "CT %u.%u",
                 CTCSS_Options[gLastActiveLoot->ct] / 10,
                 CTCSS_Options[gLastActiveLoot->ct] % 10);
  }

  PrintSmallEx(0, SPECTRUM_Y - 3 + 6, POS_L, C_FILL, "%s",
               RESET_METHOD_NAMES[rssiResetMethod]);
}
