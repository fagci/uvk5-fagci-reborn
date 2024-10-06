#include "svc_scan.h"
#include "apps/apps.h"
#include "driver/si473x.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "helper/presetlist.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"

uint16_t gScanSwitchT = 10;
bool gScanForward = true;
bool gScanRedraw = true;

uint32_t SCAN_TIMEOUTS[11] = {
    0,
    500,
    1000 * 1,
    1000 * 3,
    1000 * 5,
    1000 * 10,
    1000 * 30,
    1000 * 60,
    1000 * 60 * 2,
    1000 * 60 * 5,
    ((uint32_t)0) - 1,
};

char *SCAN_TIMEOUT_NAMES[11] = {
    "0",   "500ms", "1s",   "3s",   "5s",   "10s",
    "30s", "1min",  "2min", "5min", "None",
};

static uint32_t lastSettedF = 0;
static uint32_t timeout = 0;
static bool lastListenState = false;

void (*gScanFn)(bool) = NULL;

static void next(void) {
  lastListenState = false;
  gScanFn(gScanForward);
  lastSettedF = radio->rx.f;
  SetTimeout(&timeout, gSettings.scanTimeout);
  if (gScanRedraw) {
    gRedrawScreen = true;
  }
}

void SVC_SCAN_Init(void) {
  gScanForward = true;

  if (RADIO_GetRadio() == RADIO_SI4732) {
    SI47XX_Seek(gScanForward, true);
    return;
  }

  if (!gScanFn) {
    if (radio->channel >= 0) {
      gScanFn = RADIO_NextCH;
    } else {
      if (gSettings.crossBandScan) {
        gScanFn = RADIO_NextPresetFreqXBand;
      } else {
        gScanFn = RADIO_NextPresetFreq;
      }
    }
  }
  next();
}

uint32_t lastSavedF = 0;
bool lastScanForward = true;

void SVC_SCAN_Update(void) {
  if (RADIO_GetRadio() != RADIO_BK4819) {
    if (RADIO_GetRadio() == RADIO_SI4732 && lastScanForward != gScanForward) {
      SI47XX_Seek(gScanForward, true);
      lastScanForward = gScanForward;
    }
    return;
  }
  if (lastListenState != gIsListening) {
    if (gIsListening &&
        (gCurrentApp != APP_SPECTRUM && gCurrentApp != APP_ANALYZER &&
         gCurrentApp != APP_CH_SCANNER) &&
        lastSavedF != radio->rx.f) {
      lastSavedF = radio->rx.f;
      RADIO_SaveCurrentVFO();
    }
    lastListenState = gIsListening;
    SetTimeout(&timeout, gIsListening
                             ? SCAN_TIMEOUTS[gSettings.sqOpenedTimeout]
                             : SCAN_TIMEOUTS[gSettings.sqClosedTimeout]);
  }

  if (CheckTimeout(&timeout)) {
    next();
    return;
  }

  if (lastSettedF != radio->rx.f) {
    SetTimeout(&timeout, 0);
  }
}

void SVC_SCAN_Deinit(void) {
  gScanFn = NULL;
  gScanRedraw = true;
  sprintf(defaultPreset.band.name, "default");
  defaultPreset.band.bounds.start = 0;
  defaultPreset.band.bounds.end = 130000000;
  if (RADIO_GetRadio() != RADIO_BK4819) {
    uint32_t f = radio->rx.f;

    if (RADIO_GetRadio() == RADIO_SI4732) {
      SI47xx_GetStatus(true, true);
    }
    RADIO_TuneToSave(f);
  }
}
