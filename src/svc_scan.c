#include "svc_scan.h"
#include "driver/st7565.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"

uint16_t gScanSwitchT = 10;
bool gScanForward = true;
bool gScanRedraw = true;

uint32_t SCAN_TIMEOUTS[9] = {
    1000 * 1,  1000 * 2,      1000 * 5,      1000 * 10,         1000 * 30,
    1000 * 60, 1000 * 60 * 2, 1000 * 60 * 5, ((uint32_t)0) - 1,
};

char *SCAN_TIMEOUT_NAMES[9] = {
    "1s", "2s", "5s", "10s", "30s", "1min", "2min", "5min", "None",
};

static uint32_t lastSettedF = 0;
static uint32_t timeout = 0;
static bool lastListenState = false;

void (*gScanFn)(bool) = RADIO_NextPresetFreq;

static void next(void) {
  lastListenState = false;
  gScanFn(gScanForward);
  lastSettedF = gCurrentVFO->fRX;
  SetTimeout(&timeout, gSettings.scanTimeout);
  if (gScanRedraw) {
    gRedrawScreen = true;
  }
}

void SVC_SCAN_Init(void) {
  gScanForward = true;
  next();
}

void SVC_SCAN_Update(void) {
  if (lastListenState != gIsListening) {
    lastListenState = gIsListening;
    SetTimeout(&timeout, gIsListening
                             ? SCAN_TIMEOUTS[gSettings.sqOpenedTimeout]
                             : SCAN_TIMEOUTS[gSettings.sqClosedTimeout]);
  }

  if (CheckTimeout(&timeout)) {
    next();
    return;
  }

  if (lastSettedF != gCurrentVFO->fRX) {
    SetTimeout(&timeout, 0);
  }
}

void SVC_SCAN_Deinit(void) {
  gScanFn = RADIO_NextPresetFreq;
  gScanRedraw = true;
}
