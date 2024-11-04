#include "spectrumreborn.h"
#include "../board.h"
#include "../dcs.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../helper/channels.h"
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
#include <stdint.h>

static uint8_t noiseOpenDiff = 14;

static Band *currentBand;

static uint32_t currentStepSize;

static bool newScan = true;

static uint16_t rssiO = UINT16_MAX;
static uint8_t noiseO = 0;

static uint8_t msmDelay = 5;
static Loot msm = {0};

static uint16_t oldPresetIndex = 255;

static bool isSquelchOpen() { return msm.rssi >= rssiO && msm.noise <= noiseO; }

static void updateStats() {
  const uint16_t noiseFloor = SP_GetNoiseFloor();
  const uint8_t noiseMax = SP_GetNoiseMax();
  rssiO = noiseFloor;
  noiseO = noiseMax - noiseOpenDiff;
}

static void updateMsm() {
  if (!gIsListening) {
    BK4819_SetFrequency(msm.f);
    BK4819_WriteRegister(BK4819_REG_30, 0x0200);
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

  LOOT_Update(&msm);
  SP_AddPoint(&msm);
  RADIO_ToggleRX(msm.open);
}

static void scanFn(bool _) {
  if (msm.rssi == 0) {
    return;
  }
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
  msm.rssi = 0;
}

static void init() {
  oldPresetIndex = 0;
  rssiO = UINT16_MAX;
  noiseO = 0;

  radio->radio = RADIO_BK4819;

  LOOT_Standby();
  RADIO_SetupBandParams();

  SP_Init(&gCurrentPreset->band);
}

static void startNewScan() {
  currentBand = &gCurrentPreset->band;
  currentStepSize = PRESETS_GetStepSize(gCurrentPreset);

  msm.f = currentBand->bounds.start;

  if (gSettings.activePreset != oldPresetIndex) {
    init();
    oldPresetIndex = gSettings.activePreset;
    gLastActiveLoot = NULL;
  } else {
    SP_Begin();
  }
}

static void loadBlacklist() {
  uint8_t scanlistMask = 1 << 7;
  for (int16_t i = 0; i < CHANNELS_GetCountMax(); ++i) {
    if (!CHANNELS_Existing(i)) {
      continue;
    }
    if ((CHANNELS_Scanlists(i) & scanlistMask) == scanlistMask) {
      CH ch;
      CHANNELS_Load(i, &ch);
      Loot *loot = LOOT_AddEx(ch.rx.f, true);
      loot->open = false;
      loot->blacklist = true;
      loot->lastTimeOpen = 0;
    }
  }
}

void SPECTRUM_init(void) {
  loadBlacklist();

  SVC_Toggle(SVC_LISTEN, false, 0);
  RADIO_LoadCurrentVFO();
  newScan = true;
}

void SPECTRUM_deinit() {
  BOARD_ToggleGreen(false);
  SVC_Toggle(SVC_SCAN, false, msmDelay);
  BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);
  RADIO_ToggleRX(false);
  SVC_Toggle(SVC_LISTEN, true, 10);
  RADIO_SetupBandParams();
}

bool SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (Key) {
    case KEY_UP:
      PRESETS_SelectPresetRelative(true);
      RADIO_SelectPresetSave(gSettings.activePreset);
      startNewScan();
      return true;
    case KEY_DOWN:
      PRESETS_SelectPresetRelative(false);
      RADIO_SelectPresetSave(gSettings.activePreset);
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
      LOOT_WhitelistLast();
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
static uint32_t lastMsmUpdate = 0;
void SPECTRUM_update(void) {
  BOARD_ToggleGreen(gIsListening);
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
  if (Now() - lastMsmUpdate >= msmDelay) {
    updateMsm();
    lastMsmUpdate = Now();
  }
}

void SPECTRUM_render(void) {
  UI_ClearScreen();
  STATUSLINE_SetText(currentBand->name);

  SP_Render(gCurrentPreset);
  UI_DrawSpectrumElements(SPECTRUM_Y, msmDelay, noiseOpenDiff, currentBand);

  const uint8_t bl = 16 + 6;
  if (gLastActiveLoot) {
    if (gLastActiveLoot->ct != 0xFF) {
      PrintSmallEx(LCD_XCENTER, bl, POS_C, C_FILL, "CT %u.%u",
                   CTCSS_Options[gLastActiveLoot->ct] / 10,
                   CTCSS_Options[gLastActiveLoot->ct] % 10);
    } else if (gLastActiveLoot->cd != 0xFF) {
      PrintSmallEx(LCD_XCENTER, bl, POS_C, C_FILL, "D%03oN",
                   DCS_Options[gLastActiveLoot->cd]);
    }
  }
}
