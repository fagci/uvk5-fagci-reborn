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
#include "../ui/graphics.h"
#include "../ui/spectrum.h"
#include "../ui/statusline.h"
#include "apps.h"
#include <stdint.h>

static const uint8_t ANALYZER_Y = 16;
static const uint8_t ANALYZER_HEIGHT = 40;
static const uint8_t spectrumWidth = LCD_WIDTH;

static Loot msm;
static uint32_t centerF = 0;
static uint8_t initialScanInterval = 0;
static uint8_t scanInterval = 2;
static uint8_t stepsCount = 128;

static bool isListening = false;

static uint32_t peakF = 0;
static uint32_t _peakF = 0;
static uint16_t peakRssi = 0;

static uint16_t squelchRssi = UINT16_MAX;

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
    squelchRssi = UINT16_MAX;
    LOOT_Standby();
    msm.f = opt.band.bounds.start;
    BK4819_TuneTo(msm.f, true);
    BK4819_SetRegValue(afcDisableRegSpec, true);
    BK4819_SetModulation(opt.band.modulation);
    if (step > 5000) {
      BK4819_SetModulation(MOD_WFM);
    } else if (step >= 2400) {
      BK4819_SetFilterBandwidth(BK4819_FILTER_BW_WIDE);
    } else if (step >= 1200) {
      BK4819_SetFilterBandwidth(BK4819_FILTER_BW_NARROW);
    } else {
      BK4819_SetFilterBandwidth(BK4819_FILTER_BW_NARROWER);
    }
    SP_Init(PRESETS_GetSteps(&opt), spectrumWidth);
  } else {
    SP_Begin();
  }
}

static void scanFn(bool forward) {
  msm.rssi = RADIO_GetRSSI();
  SP_AddPoint(&msm);

  if (msm.rssi > peakRssi) {
    peakRssi = msm.rssi;
    _peakF = msm.f;
  }

  RADIO_ToggleRX(msm.rssi > squelchRssi);
  if (gIsListening) {
    return;
  }

  if (msm.f + step > opt.band.bounds.end) {
    msm.f = opt.band.bounds.start;
    peakF = _peakF;
    gRedrawScreen = true;
    if (squelchRssi == UINT16_MAX) {
      squelchRssi = SP_GetRssiMax() + 2;
    }
  } else {
    msm.f += step;
  }

  RADIO_TuneToPure(msm.f, false);

  if (msm.f == opt.band.bounds.start) {
    startNewScan(false);
    return;
  }
  SP_Next();
}

static void setCenterF(uint32_t f) {
  step = StepFrequencyTable[opt.band.step];
  const uint32_t halfBW = step * (stepsCount / 2);
  centerF = f;
  opt.band.bounds.start = centerF - halfBW;
  opt.band.bounds.end = centerF + halfBW;
  startNewScan(true);
}

static void setup(uint32_t f) {
  setCenterF(f);
  gSettings.scanTimeout = scanInterval;
}

void ANALYZER_init(void) {
  SVC_Toggle(SVC_LISTEN, false, 0);
  RADIO_ToggleRX(false);
  RADIO_LoadCurrentVFO();

  gMonitorMode = false;

  initialScanInterval = gSettings.scanTimeout;
  opt.band.step = gCurrentPreset->band.step;
  opt.band.squelch = 0;

  setup(radio->rx.f);
  startNewScan(true);

  gScanFn = scanFn;
  SVC_Toggle(SVC_SCAN, true, scanInterval);
  gScanRedraw = false;
}

void ANALYZER_update(void) {}

void ANALYZER_deinit(void) {
  SVC_Toggle(SVC_SCAN, false, 0);
  gSettings.scanTimeout = initialScanInterval;
  SVC_Toggle(SVC_LISTEN, true, 1);
}

bool ANALYZER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  // repeat or keyup
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (Key) {
    case KEY_1:
      IncDec8(&scanInterval, 1, 255, 1);
      setup(centerF);
      return true;
    case KEY_7:
      IncDec8(&scanInterval, 1, 255, -1);
      setup(centerF);
      return true;
    default:
      break;
    }
  }

  // long press
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (Key) {
    case KEY_UP:
      setCenterF(centerF + StepFrequencyTable[opt.band.step] * 64);
      return true;
    case KEY_DOWN:
      setCenterF(centerF - StepFrequencyTable[opt.band.step] * 64);
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
      setCenterF(centerF + StepFrequencyTable[opt.band.step]);
      return true;
    case KEY_DOWN:
      setCenterF(centerF - StepFrequencyTable[opt.band.step]);
      return true;
    case KEY_SIDE1:
      isListening ^= 1;
      if (isListening) {
        RADIO_TuneToPure(centerF, true);
      }
      RADIO_ToggleRX(isListening);
      return true;
    case KEY_SIDE2:
      if (peakF) {
        setCenterF(peakF);
      }
      return true;
    case KEY_2:
      if (opt.band.step < STEP_200_0kHz) {
        opt.band.step++;
      }
      setup(centerF);
      return true;
    case KEY_8:
      if (opt.band.step > 0) {
        opt.band.step--;
      }
      setup(centerF);
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
    case KEY_PTT:
      RADIO_TuneToSave(centerF);
      APPS_run(APP_STILL);
      return true;
    default:
      break;
    }
  }
  return false;
}

void ANALYZER_render(void) {
  UI_ClearScreen();
  STATUSLINE_SetText(opt.band.name);

  for (uint8_t i = ANALYZER_Y; i < ANALYZER_Y + ANALYZER_HEIGHT; i += 4) {
    PutPixel(spectrumWidth / 2, i, C_FILL);
  }

  SP_Render(&opt, 0, ANALYZER_Y, ANALYZER_HEIGHT);
  if (squelchRssi != UINT16_MAX) {
    SP_RenderRssi(squelchRssi, "SQ", true, 0, ANALYZER_Y, ANALYZER_HEIGHT);
  }

  SP_RenderArrow(&opt, peakF, 0, ANALYZER_Y, ANALYZER_HEIGHT);

  PrintSmallEx(spectrumWidth - 2, ANALYZER_Y - 3, POS_R, C_FILL, "%04d/%04d",
               Rssi2DBm(SP_GetNoiseFloor()), Rssi2DBm(SP_GetRssiMax()));
  PrintSmallEx(0, ANALYZER_Y - 3, POS_L, C_FILL, "%ums", scanInterval);
  PrintSmallEx(LCD_XCENTER, ANALYZER_Y - 3, POS_C, C_FILL, "Stp %u.%02uk",
               StepFrequencyTable[opt.band.step] / 100,
               StepFrequencyTable[opt.band.step] % 100);

  PrintSmallEx(spectrumWidth / 2, LCD_HEIGHT - 1, POS_C, C_FILL, "%u.%05u",
               centerF / 100000, centerF % 100000);
}
