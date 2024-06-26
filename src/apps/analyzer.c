#include "analyzer.h"
#include "../driver/st7565.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../settings.h"
#include "../svc.h"
#include "../svc_scan.h"
#include "../ui/graphics.h"
#include "../ui/spectrum.h"
#include "../ui/statusline.h"
#include "apps.h"

static const uint8_t ANALYZER_Y = 16;
static const uint8_t ANALYZER_HEIGHT = 40;

static Loot msm;
static uint32_t centerF = 0;
static uint8_t initialScanInterval = 0;
static uint8_t scanInterval = 2;

static bool isListening = false;

static uint32_t peakF = 0;
static uint32_t _peakF = 0;
static uint16_t peakRssi = 0;

static Preset opt = {
    .band =
        {
            .name = "Analyzer",
        },
};

static uint8_t spectrumWidth = LCD_WIDTH;

static bool bandFilled = false;

static void startNewScan(bool reset) {
  _peakF = 0;
  peakRssi = 0;
  if (reset) {
    LOOT_Standby();
    RADIO_TuneToPure(msm.f = opt.band.bounds.start, true);
    SP_Init(PRESETS_GetSteps(&opt), spectrumWidth);
    bandFilled = false;
  } else {
    SP_Begin();
    bandFilled = true;
  }
}

static void scanFn(bool forward) {
  msm.rssi = RADIO_GetRSSI();
  SP_AddPoint(&msm);

  if (msm.rssi > peakRssi) {
    peakRssi = msm.rssi;
    _peakF = msm.f;
  }

  uint16_t step = StepFrequencyTable[opt.band.step];

  if (msm.f + step > opt.band.bounds.end) {
    msm.f = opt.band.bounds.start;
    peakF = _peakF;
    gRedrawScreen = true;
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
  const uint32_t halfBW = StepFrequencyTable[opt.band.step] * 64;
  centerF = f;
  opt.band.bounds.start = centerF - halfBW;
  opt.band.bounds.end = centerF + halfBW;
}

static void setup(void) {
  setCenterF(radio->rx.f);
  gSettings.scanTimeout = scanInterval;
  startNewScan(true);
}

void ANALYZER_init(void) {
  SVC_Toggle(SVC_LISTEN, false, 0);
  RADIO_ToggleRX(false);
  RADIO_LoadCurrentVFO();
  // RADIO_ToggleBK1080(false);

  gMonitorMode = false;

  initialScanInterval = gSettings.scanTimeout;
  opt.band.step = gCurrentPreset->band.step;
  opt.band.squelch = 0;

  setup();
  startNewScan(true);

  gScanFn = scanFn;
  SVC_Toggle(SVC_SCAN, true, 1);
  gScanRedraw = false;

  gRedrawScreen = true;
}

void ANALYZER_update(void) {}

void ANALYZER_deinit(void) {
  SVC_Toggle(SVC_SCAN, false, 0);
  gSettings.scanTimeout = initialScanInterval;
  SVC_Toggle(SVC_LISTEN, true, 1);
}

bool ANALYZER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {

  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (Key) {
    case KEY_1:
      IncDec8(&scanInterval, 1, 255, 1);
      setup();
      return true;
    case KEY_7:
      IncDec8(&scanInterval, 1, 255, -1);
      setup();
      return true;
    default:
      break;
    }
  }

  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (Key) {
    case KEY_0:
      LOOT_Clear();
      return true;
    case KEY_UP:
      centerF += StepFrequencyTable[opt.band.step] * 64;
      setup();
      return true;
    case KEY_DOWN:
      centerF -= StepFrequencyTable[opt.band.step] * 64;
      setup();
      return true;
    default:
      break;
    }
  }

  if (!bKeyPressed && !bKeyHeld) {
    switch (Key) {
    case KEY_EXIT:
      APPS_exit();
      return true;
    case KEY_UP:
      setCenterF(centerF + StepFrequencyTable[opt.band.step]);
      startNewScan(true);
      return true;
    case KEY_DOWN:
      setCenterF(centerF - StepFrequencyTable[opt.band.step]);
      startNewScan(true);
      return true;
    case KEY_SIDE1:
      isListening = !isListening;
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
      setup();
      return true;
    case KEY_8:
      if (opt.band.step > 0) {
        opt.band.step--;
      }
      setup();
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
    case KEY_3:
      if (opt.band.squelch < 9) {
        opt.band.squelch++;
      }
      return true;
    case KEY_9:
      if (opt.band.squelch > 0) {
        opt.band.squelch--;
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

  SP_RenderArrow(&opt, peakF, 0, ANALYZER_Y, ANALYZER_HEIGHT);

  if (opt.band.squelch) {
    uint8_t band = centerF > SETTINGS_GetFilterBound() ? 1 : 0;
    SP_RenderRssi(SQ[band][0][opt.band.squelch], "SQLo", true, 0, ANALYZER_Y,
                  ANALYZER_HEIGHT);
    SP_RenderRssi(SQ[band][1][opt.band.squelch], "SQLc", false, 0, ANALYZER_Y,
                  ANALYZER_HEIGHT);
  }

  PrintSmallEx(spectrumWidth - 2, ANALYZER_Y - 3, POS_R, C_FILL, "SQ:%u",
               opt.band.squelch);
  PrintSmallEx(0, ANALYZER_Y - 3 + 6, POS_L, C_FILL, "%ums", scanInterval);
  PrintSmallEx(LCD_XCENTER, ANALYZER_Y - 3, POS_C, C_FILL, "Step: %u.%02uk",
               StepFrequencyTable[opt.band.step] / 100,
               StepFrequencyTable[opt.band.step] % 100);

  PrintSmallEx(spectrumWidth / 2, LCD_HEIGHT - 1, POS_C, C_FILL, "%u.%05u",
               centerF / 100000, centerF % 100000);
}
