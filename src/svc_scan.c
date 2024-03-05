#include "svc_scan.h"
#include "driver/st7565.h"
#include "driver/system.h"
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
    1000 * 2,
    1000 * 5,
    1000 * 10,
    1000 * 30,
    1000 * 60,
    1000 * 60 * 2,
    1000 * 60 * 5,
    ((uint32_t)0) - 1,
};

char *SCAN_TIMEOUT_NAMES[11] = {
    "0",   "500ms", "1s",   "2s",   "5s",   "10s",
    "30s", "1min",  "2min", "5min", "None",
};

static uint32_t lastSettedF = 0;
static uint32_t timeout = 0;
static bool lastListenState = false;

void (*gScanFn)(bool) = NULL;

static void next() {
  lastListenState = false;
  gScanFn(gScanForward);
  lastSettedF = radio->f;
  SetTimeout(&timeout, radio->scan.timeout);
  if (gScanRedraw) {
    gRedrawScreen = true;
  }
}

#include "driver/uart.h"
void SVC_SCAN_Init() {
  gScanForward = true;
  Log("SCAN init, SF:%u", !!gScanFn);
  if (!gScanFn) {
    if (radio->channel >= 0) {
      gScanFn = RADIO_NextCH;
    } else {
      gScanFn = RADIO_NextBandFreq;
    }
  }
  next();
}

void SVC_SCAN_Update() {
  if (lastListenState != gIsListening) {
    lastListenState = gIsListening;
    SetTimeout(&timeout, gIsListening
                             ? SCAN_TIMEOUTS[radio->scan.openedTimeout]
                             : SCAN_TIMEOUTS[radio->scan.closedTimeout]);
  }

  if (CheckTimeout(&timeout)) {
    next();
    return;
  }

  if (lastSettedF != radio->f) {
    SetTimeout(&timeout, 0);
  }
}

void SVC_SCAN_Deinit() {
  gScanFn = NULL;
  gScanRedraw = true;
}
