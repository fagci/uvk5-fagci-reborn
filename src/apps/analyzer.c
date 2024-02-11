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
static uint32_t initialF = 0;
static uint32_t centerF = 0;
static uint8_t initialScanInterval = 0;
static uint8_t scanInterval = 2;

static Preset opt = {
    .band =
        {
            .name = "Analyzer",
        },
};

static uint8_t spectrumWidth = LCD_WIDTH;

static bool newScan = false;
static bool bandFilled = false;

static uint32_t lastRender = 0;

static void startNewScan(bool reset) {
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

static void nextF(void) {
  uint16_t step = StepFrequencyTable[opt.band.step];

  if (msm.f + step > opt.band.bounds.end) {
    msm.f = opt.band.bounds.start;
  } else {
    msm.f += step;
  }

  RADIO_TuneToPure(msm.f, false);
}

static void scanFn(bool forward) {
  msm.rssi = RADIO_GetRSSI();

  SP_AddPoint(&msm);

  if (newScan) {
    newScan = false;
    startNewScan(false);
  }

  nextF();

  if (msm.f == opt.band.bounds.start) {
    startNewScan(false);
    return;
  } else if (msm.f + StepFrequencyTable[opt.band.step] > opt.band.bounds.end) {
    gRedrawScreen = true;
  }
  SP_Next();
}

static void setup(void) {
  const uint32_t bandwidth = StepFrequencyTable[opt.band.step] * 128;
  opt.band.bounds.start = centerF - bandwidth / 2;
  opt.band.bounds.end = centerF + bandwidth / 2;
  gSettings.scanTimeout = scanInterval;
  newScan = true;
}

void ANALYZER_init(void) {
  SVC_Toggle(SVC_LISTEN, false, 0);
  RADIO_ToggleRX(false);
  RADIO_LoadCurrentVFO();
  gMonitorMode = false;
  gNoListen = true;

  initialF = centerF = gCurrentVFO->fRX;
  initialScanInterval = gSettings.scanTimeout;
  opt.band.step = gCurrentPreset->band.step;
  opt.band.squelch = gCurrentPreset->band.squelch;

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
  gNoListen = false;
  gSettings.scanTimeout = initialScanInterval;
  RADIO_TuneTo(initialF);
  SVC_Toggle(SVC_LISTEN, true, 1);
}

bool ANALYZER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    if (Key == KEY_SIDE1) {
      gNoListen = !gNoListen;
      RADIO_ToggleRX(false);
      return true;
    }
    if (Key == KEY_0) {
      LOOT_Clear();
      return true;
    }
  }

  if (!bKeyPressed && !bKeyHeld) {
    switch (Key) {
    case KEY_EXIT:
      APPS_exit();
      return true;
    case KEY_UP:
      centerF += StepFrequencyTable[opt.band.step];
      setup();
      startNewScan(true);
      return true;
    case KEY_DOWN:
      centerF -= StepFrequencyTable[opt.band.step];
      setup();
      startNewScan(true);
      return true;
    case KEY_2:
      if (opt.band.step < STEP_200_0kHz) {
        opt.band.step++;
      }
      setup();
      startNewScan(true);
      return true;
    case KEY_8:
      if (opt.band.step > 0) {
        opt.band.step--;
      }
      setup();
      startNewScan(true);
      return true;
    case KEY_SIDE1:
      LOOT_BlacklistLast();
      return true;
    case KEY_SIDE2:
      LOOT_GoodKnownLast();
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
      RADIO_UpdateSquelchLevel(true);
      startNewScan(true);
      return true;
    case KEY_9:
      if (opt.band.squelch > 1) {
        RADIO_UpdateSquelchLevel(false);
        startNewScan(true);
      }
      return true;
    case KEY_PTT:
      RADIO_TuneToSave(centerF);
      APPS_run(APP_STILL);
      return true;
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
  return false;
}

void ANALYZER_render(void) {
  UI_ClearScreen();
  STATUSLINE_SetText(opt.band.name);

  for (uint8_t i = ANALYZER_Y; i < ANALYZER_Y + ANALYZER_HEIGHT; i += 4) {
    PutPixel(spectrumWidth / 2, i, C_FILL);
  }

  SP_Render(&opt, 0, ANALYZER_Y, ANALYZER_HEIGHT);

  PrintSmallEx(spectrumWidth - 2, ANALYZER_Y - 3, POS_R, C_FILL, "SQ:%u",
               opt.band.squelch);
  if (gNoListen) {
    PrintSmallEx(0, ANALYZER_Y - 3, POS_L, C_FILL, "No listen");
  }
  PrintSmallEx(0, ANALYZER_Y - 3 + 6, POS_L, C_FILL, "%ums", scanInterval);
  PrintSmallEx(LCD_WIDTH, ANALYZER_Y - 3 + 6, POS_R, C_FILL, "Step: %u.%02uk",
               StepFrequencyTable[opt.band.step] / 100,
               StepFrequencyTable[opt.band.step] % 100);

  PrintSmallEx(spectrumWidth / 2, LCD_HEIGHT - 1, POS_C, C_FILL, "%u.%05u",
               centerF / 100000, centerF % 100000);

  lastRender = elapsedMilliseconds;
}
