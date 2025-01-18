#include "scan.h"
#include "../driver/uart.h"
#include "../external/printf/printf.h"
#include "../helper/bands.h"
#include "../radio.h"
#include "../svc.h"
#include "../svc_scan.h"
#include "../ui/spectrum.h"
#include "channels.h"

// 50 was default (ok), 60 is better, choose mid =)
static const uint32_t DEFAULT_SCAN_TIMEOUT = 55;

static uint32_t scanTimeout = DEFAULT_SCAN_TIMEOUT;

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
  scanTimeout = DEFAULT_SCAN_TIMEOUT;
  gScanFn = CHANNELS_Next;
}

static void initSsbScan() {
  scanTimeout = 150;
  gScanFn = RADIO_NextBandFreqXBand;
}

static void startScan() {
  scanTimeout = gSettings.scanTimeout; // fast freq scan
  SCAN_UpdateOpenLevel();
  if (gScanlistSize == 0 && !RADIO_IsSSB()) {
    SCAN_Stop();
    return;
  }

  if (RADIO_IsChMode()) {
    initChannelScan();
  }

  SVC_Toggle(SVC_SCAN, true, scanTimeout);
  SVC_Toggle(SVC_LISTEN, false, 0);
  SVC_Toggle(SVC_LISTEN, true, 0);
}

bool SCAN_IsFast() { return scanTimeout < 10; }

uint32_t SCAN_GetTimeout() { return scanTimeout; }

void SCAN_UpdateOpenLevel() {
  gNoiseOpenDiff =
      scanTimeout < 10
          ? (uint8_t[]){20, 22, 36, 45, 48, 50, 52, 54, 58, 62}[scanTimeout]
          : (50 + scanTimeout / 10);
}

void SCAN_UpdateTimeoutFromSetting() {
  scanTimeout = gSettings.scanTimeout;
  SVC_Toggle(SVC_SCAN, false, scanTimeout);
  SVC_Toggle(SVC_SCAN, true, scanTimeout);
}

void SCAN_ToggleDirection(bool up) {
  if (RADIO_IsChMode()) {
    CHANNELS_Next(up);
    RADIO_SaveCurrentVFODelayed();
    return;
  }
  if (!gIsListening && SVC_Running(SVC_SCAN)) {
    if (gScanForward == up) {
      BANDS_SelectBandRelativeByScanlist(up);
      return;
    }
    gScanForward = up;
  }
  RADIO_NextBandFreqXBandEx(up, false);
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
    if (RADIO_IsChMode() && gScanlistSize == 0) {
      return;
    }
    startScan();
    // NOTE: important to tune to scanlist band,
    // not to band by frequency in radio.h
    if (!RADIO_IsChMode()) {
      if (gSettings.currentScanlist) {
        BANDS_SelectScan(0);
      }
      if (!radio->fixedBoundsMode) {
        radio->fixedBoundsMode = true;
        radio->step = gCurrentBand.step;
        RADIO_SaveCurrentVFO();
      }
    }
  }
}

void SCAN_Stop() {
  scanTimeout = DEFAULT_SCAN_TIMEOUT;
  SVC_Toggle(SVC_SCAN, false, 0);
  SVC_Toggle(SVC_LISTEN, false, 0);
  SVC_Toggle(SVC_LISTEN, true, 10);
}
