#include "scan.h"
#include "../driver/uart.h"
#include "../external/printf/printf.h"
#include "../helper/bands.h"
#include "../radio.h"
#include "../svc.h"
#include "../svc_scan.h"
#include "channels.h"

// 50 was default (ok), 60 is better, choose mid =)
static const uint32_t DEFAULT_SCAN_TIMEOUT = 55;

static int16_t scanIndex = 0;
static uint32_t scanTimeout = DEFAULT_SCAN_TIMEOUT;

static void channelScanFn(bool forward) {
  IncDecI16(&scanIndex, 0, gScanlistSize, forward ? 1 : -1);
  int16_t chNum = gScanlist[scanIndex];
  radio->channel = chNum;
  RADIO_VfoLoadCH(gSettings.activeVFO);
  RADIO_SetupByCurrentVFO();
}

void prepareABScan() {
  uint32_t F1 = gVFO[0].rxF;
  uint32_t F2 = gVFO[1].rxF;

  if (F1 > F2) {
    SWAP(F1, F2);
  }

  gCurrentBand = defaultBand;

  gCurrentBand.meta.type = TYPE_BAND_DETACHED;
  gCurrentBand.rxF = F1;
  gCurrentBand.txF = F2;
  gCurrentBand.step = radio->step;

  sprintf(gCurrentBand.name, "%u-%u", F1 / MHZ, F2 / MHZ);
  RADIO_TuneToPure(F1, true);
  radio->fixedBoundsMode = true;
  // RADIO_SaveCurrentVFO();
}

static void initChannelScan() {
  scanIndex = 0;
  scanTimeout = DEFAULT_SCAN_TIMEOUT;
  gScanFn = channelScanFn;
}

static void initSsbScan() {
  scanTimeout = 150;
  gScanFn = RADIO_NextBandFreqXBand;
}

static void startScan() {
  scanTimeout = gSettings.scanTimeout; // fast freq scan
  Log("TO: %u", scanTimeout);
  if (gScanlistSize == 0 && !RADIO_IsSSB()) {
    SCAN_Stop();
    return;
  }

  if (RADIO_IsChMode()) {
    initChannelScan();
  }

  SVC_Toggle(SVC_SCAN, true, scanTimeout);
}

bool SCAN_IsFast() { return scanTimeout < 10; }

uint32_t SCAN_GetTimeout() { return scanTimeout; }

void SCAN_ToggleDirection(bool up) {
  if (!gIsListening && SVC_Running(SVC_SCAN)) {
    if (gScanForward == up) {
      BANDS_SelectBandRelativeByScanlist(up);
      return;
    }
    gScanForward = up;
  }
  RADIO_NextBandFreqXBand(up);
}

void SCAN_StartAB() {
  prepareABScan();
  startScan();
}

void SCAN_Start() {
  if (RADIO_GetRadio() == RADIO_SI4732 && RADIO_IsSSB()) {
    // TODO: scan by snr
    initSsbScan();
  } else {
    startScan();
    // NOTE: important to tune to scanlist band,
    // not to band by frequency in radio.h
    if (!RADIO_IsChMode()) {
      if (gSettings.currentScanlist) {
        BANDS_SelectScan(0);
      }
      if (!radio->fixedBoundsMode) {
        radio->fixedBoundsMode = true;
        RADIO_SaveCurrentVFO();
      }
    }
  }
}

void SCAN_Stop() {
  scanTimeout = DEFAULT_SCAN_TIMEOUT;
  SVC_Toggle(SVC_SCAN, false, 0);
}
