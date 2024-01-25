#include "svc_scan.h"
#include "radio.h"
#include "scheduler.h"

uint16_t switchT = 10;
uint16_t activeT = 5000;
uint16_t inactiveT = 3000;
bool gScanForward = true;

void (*gScanFn)(bool) = NULL;

static uint32_t timeout = 0;
static bool lastListenState = false;

void SVC_SCAN_Init(void) {
  gScanForward = true;
  if (!gScanFn) {
    gScanFn = RADIO_NextPresetFreq;
  }
  gScanFn(gScanForward);
  SetTimeout(&timeout, switchT);
}

void SVC_SCAN_Update(void) {
  RADIO_UpdateMeasurements();
  if (lastListenState != gIsListening) {
    lastListenState = gIsListening;
    SetTimeout(&timeout, gIsListening ? activeT : inactiveT);
  }

  if (CheckTimeout(&timeout)) {
    gScanFn(gScanForward);
    SetTimeout(&timeout, switchT);
    lastListenState = false;
  }
}

void SVC_SCAN_Deinit(void) {
  // TODO: restore some freq
  gScanFn = NULL; // to make simple scan on start
}
