#include "analyzer.h"
#include "../apps/finput.h"
#include "../driver/st7565.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../radio.h"
#include "../settings.h"
#include "../svc.h"
#include "../svc_scan.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/spectrum.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "vfo1.h"
#include <stdint.h>

static const uint8_t spectrumWidth = LCD_WIDTH;

static Loot msm;
static uint32_t centerF = 0;
static uint8_t scanInterval = 2;
static uint8_t stepsCount = 128;

static uint32_t peakF = 0;
static uint32_t _peakF = 0;
static uint16_t peakRssi = 0;

static uint16_t squelchRssi = UINT16_MAX;
static bool useSquelch = false;

static Preset opt = {
    .band =
        {
            .name = "Analyzer",
        },
};
static uint16_t step;

static void startNewScan(bool reset) {
  _peakF = 0;
  peakRssi = 0;
  if (reset) {
    LOOT_Standby();
    msm.f = opt.band.rxF;
    BK4819_TuneTo(msm.f, true);
    BK4819_SetRegValue(afcDisableRegSpec, true);
    BK4819_SetModulation(opt.modulation);
    if (step > 5000) {
      BK4819_SetModulation(MOD_WFM);
    } else if (step >= 1200) {
      BK4819_SetFilterBandwidth(BK4819_FILTER_BW_WIDE);
    } else if (step >= 625) {
      BK4819_SetFilterBandwidth(BK4819_FILTER_BW_NARROW);
    } else {
      BK4819_SetFilterBandwidth(BK4819_FILTER_BW_NARROWER);
    }
    SP_Init(&opt.band);
  } else {
    SP_Begin();
  }
}

static void scanFn(bool forward) {
  msm.rssi = RADIO_GetRSSI();
  if (gMonitorMode) {
    msm.open = true;
  } else {
    msm.open = msm.rssi > squelchRssi;
    LOOT_Update(&msm);
  }
  SP_AddPoint(&msm);

  if (msm.rssi > peakRssi) {
    peakRssi = msm.rssi;
    _peakF = msm.f;
  }

  RADIO_ToggleRX(msm.open);
  if (gIsListening) {
    return;
  }

  if (msm.f + step > opt.band.txF) {
    msm.f = opt.band.rxF;
    peakF = _peakF;
    gRedrawScreen = true;
    if (useSquelch && squelchRssi == UINT16_MAX) {
      squelchRssi = SP_GetRssiMax() + 2;
    }
  } else {
    msm.f += step;
  }

  RADIO_TuneToPure(msm.f, false);

  if (msm.f == opt.band.rxF) {
    startNewScan(false);
    return;
  }
  SP_Next();
}

static void setCenterF(uint32_t f) {
  step = StepFrequencyTable[opt.step];
  const uint32_t halfBW = step * (stepsCount / 2);
  centerF = f;
  opt.band.rxF = centerF - halfBW;
  opt.band.txF = centerF + halfBW;
  opt.modulation = RADIO_GetModulation();
  startNewScan(true);
}

void ANALYZER_init(void) {
  SVC_Toggle(SVC_SCAN, false, 0);
  SVC_Toggle(SVC_LISTEN, false, 0);
  RADIO_ToggleRX(false);
  RADIO_LoadCurrentVFO();

  gMonitorMode = false;

  opt.step = gCurrentPreset->step;
  opt.band.squelch = 0;

  setCenterF(radio->rx.f);
  startNewScan(true);
  squelchRssi = UINT16_MAX;

  gScanFn = scanFn;
  SVC_Toggle(SVC_SCAN, true, scanInterval);
  gScanRedraw = false;
}

bool lastListenState = false;

void ANALYZER_update(void) {
  if (gIsListening) {
    scanFn(true);
  }
}

void ANALYZER_deinit(void) {
  SVC_Toggle(SVC_SCAN, false, 0);
  SVC_Toggle(SVC_LISTEN, true, 1);
}

static void restartScan() {
  SVC_Toggle(SVC_SCAN, false, scanInterval);
  gScanFn = scanFn;
  SVC_Toggle(SVC_SCAN, true, scanInterval);
}

bool ANALYZER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  const uint32_t step = StepFrequencyTable[opt.step];

  // repeat or keyup
  if (bKeyPressed || !bKeyHeld) {
    switch (Key) {
    case KEY_1:
      IncDec8(&scanInterval, 1, 255, 1);
      restartScan();
      return true;
    case KEY_7:
      IncDec8(&scanInterval, 1, 255, -1);
      restartScan();
      return true;
    case KEY_3:
      if (squelchRssi < 512) {
        squelchRssi += 2;
      }
      return true;
    case KEY_9:
      if (squelchRssi > 1) {
        squelchRssi -= 2;
      }
      return true;
    default:
      break;
    }
  }

  // long press
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (Key) {
    case KEY_UP:
      setCenterF(centerF + step * 64);
      return true;
    case KEY_DOWN:
      setCenterF(centerF - step * 64);
      return true;
    case KEY_SIDE1:
      gMonitorMode ^= 1;
      if (gMonitorMode) {
        RADIO_TuneToPure(centerF, true);
      }
      RADIO_ToggleRX(gMonitorMode);
      return true;
    case KEY_SIDE2:
      if (useSquelch) {
        useSquelch = false;
        squelchRssi = UINT16_MAX;
      } else {
        useSquelch = true;
      }
      return true;
    default:
      break;
    }
  }

  // just pressed
  if (!bKeyPressed && !bKeyHeld) {
    switch (Key) {
    case KEY_EXIT:
      APPS_exit();
      return true;
    case KEY_UP:
      setCenterF(centerF + step);
      return true;
    case KEY_DOWN:
      setCenterF(centerF - step);
      return true;
    case KEY_SIDE1:
      LOOT_BlacklistLast();
      return true;
    case KEY_SIDE2:
      LOOT_WhitelistLast();
      return true;
    case KEY_2:
      if (opt.step < STEP_500_0kHz) {
        opt.step++;
      }
      setCenterF(centerF);
      return true;
    case KEY_8:
      if (opt.step > 0) {
        opt.step--;
      }
      setCenterF(centerF);
      return true;
    case KEY_F:
      APPS_run(APP_PRESET_CFG);
      return true;
    case KEY_0:
      if (stepsCount > 32) {
        stepsCount >>= 1;
      } else {
        stepsCount = 128;
      }
      setCenterF(centerF);
      return true;
    case KEY_STAR:
      APPS_run(APP_LOOT_LIST);
      return true;
    case KEY_5:
      gFInputCallback = setCenterF;
      APPS_run(APP_FINPUT);
      return true;
    case KEY_PTT:
      if (centerF == peakF) {
        RADIO_TuneToSave(centerF);
        gVfo1ProMode = true;
        APPS_run(APP_VFO1);
      } else if (peakF) {
        setCenterF(peakF);
      }
      return true;
    default:
      break;
    }
  }
  return false;
}

void ANALYZER_render(void) {
  STATUSLINE_SetText(opt.band.name);

  for (uint8_t i = SPECTRUM_Y; i < SPECTRUM_Y + SPECTRUM_H; i += 4) {
    PutPixel(spectrumWidth / 2, i, C_FILL);
  }

  SP_Render(&opt);

  UI_DrawSpectrumElements(SPECTRUM_Y, scanInterval, Rssi2DBm(squelchRssi),
                          &opt.band);

  SP_RenderArrow(&opt, peakF);

  if (squelchRssi != UINT16_MAX) {
    SP_RenderLine(squelchRssi);
  }

  PrintSmallEx(0, SPECTRUM_Y - 3 + 6, POS_L, C_FILL, "%u.%02uk",
               StepFrequencyTable[opt.step] / 100,
               StepFrequencyTable[opt.step] % 100);
  PrintSmallEx(LCD_XCENTER, 4, POS_C, C_FILL, "%04d/%04d",
               Rssi2DBm(SP_GetNoiseFloor()), Rssi2DBm(SP_GetRssiMax()));

  PrintSmallEx(spectrumWidth / 2, LCD_HEIGHT - 1, POS_C, C_FILL, "%u.%05u",
               centerF / MHZ, centerF % MHZ);
}
