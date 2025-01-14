#include "svc_scan.h"
#include "apps/apps.h"
#include "driver/si473x.h"
#include "driver/st7565.h"
#include "driver/uart.h"
#include "external/printf/printf.h"
#include "helper/bands.h"
#include "helper/channels.h"
#include "helper/scan.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include "ui/spectrum.h"

bool gScanForward = true;
bool gScanRedraw = true;

uint32_t SCAN_TIMEOUTS[15] = {
    0,         100,       200,           300,           400,
    500,       1000 * 1,  1000 * 3,      1000 * 5,      1000 * 10,
    1000 * 30, 1000 * 60, 1000 * 60 * 2, 1000 * 60 * 5, ((uint32_t)0) - 1,
};

char *SCAN_TIMEOUT_NAMES[15] = {
    "0",  "100ms", "200ms", "300ms", "400ms", "500ms", "1s",   "3s",
    "5s", "10s",   "30s",   "1min",  "2min",  "5min",  "None",
};

static uint32_t timeout = 0;
static uint32_t lastSettedF = 0;
static uint32_t lastSavedF = 0;
static bool lastScanForward = true;
static bool lastListenState = false;

static uint32_t lastScanRedraw;

void (*gScanFn)(bool) = NULL;

static void next(void) {
  gScanFn(gScanForward);
  // Log("SCN f=%u", radio->rxF);
  lastSettedF = radio->rxF;
  SetTimeout(&timeout, 0); // will be passed at next update
  if (gScanRedraw && Now() - lastScanRedraw > 250) {
    lastScanRedraw = Now();
    gRedrawScreen = true;
  }
}

void SVC_SCAN_Init(void) {
  gScanForward = true;

  if (RADIO_GetRadio() == RADIO_SI4732) {
    SI47XX_Seek(gScanForward, true);
    return;
  }
  SP_Init(&gCurrentBand);

  if (!gScanFn) {
    if (RADIO_IsChMode()) {
      gScanFn = CHANNELS_Next;
    } else {
      gScanFn = RADIO_NextBandFreqXBand;
    }
  }
}

void SVC_SCAN_Update(void) {
  if (RADIO_GetRadio() != RADIO_BK4819) {
    if (RADIO_GetRadio() == RADIO_SI4732 && lastScanForward != gScanForward) {
      SI47XX_Seek(gScanForward, true);
      lastScanForward = gScanForward;
    }
    return;
  }
  if (lastListenState != gIsListening) {
    if (gIsListening && (gCurrentApp != APP_ANALYZER) &&
        lastSavedF != radio->rxF) {
      lastSavedF = radio->rxF;
      RADIO_SaveCurrentVFO();
    }
    lastListenState = gIsListening;
    SetTimeout(&timeout, gIsListening
                             ? SCAN_TIMEOUTS[gSettings.sqOpenedTimeout]
                             : SCAN_TIMEOUTS[gSettings.sqClosedTimeout]);
  }

  if ((SCAN_IsFast() && gLoot[gSettings.activeVFO].snr && !gIsListening) ||
      CheckTimeout(&timeout)) {
    next();
    return;
  }

  if (lastSettedF != radio->rxF) {
    SetTimeout(&timeout, 0);
  }
}

void SVC_SCAN_Deinit(void) {
  gScanFn = NULL;
  gScanRedraw = true;
  if (RADIO_GetRadio() != RADIO_BK4819) {
    uint32_t f = radio->rxF;

    if (RADIO_GetRadio() == RADIO_SI4732) {
      SI47xx_GetStatus(true, true);
    }
    RADIO_TuneToSave(f);
  }
}
