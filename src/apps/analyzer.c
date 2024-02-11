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

static uint32_t initialF = 0;
static uint32_t centerF = 0;
static uint32_t bandwidth = 160000;
static uint8_t initialScanInterval = 0;
static uint8_t scanInterval = 2;

static Preset opt = {
    .band =
        {
            .name = "Analyzer",
        },
};

static uint8_t spectrumWidth = 84;

static bool newScan = false;
static bool bandFilled = false;

static uint32_t lastRender = 0;

static void startNewScan(bool reset) {
  if (reset) {
    LOOT_Standby();
    RADIO_TuneTo(opt.band.bounds.start);
    SP_Init(PRESETS_GetSteps(&opt), spectrumWidth);
    bandFilled = false;
  } else {
    SP_Begin();
    bandFilled = true;
  }
}

static void nextF(void) {
  uint32_t f = gCurrentVFO->fRX;

  if (f + StepFrequencyTable[opt.band.step] > opt.band.bounds.end) {
    f = opt.band.bounds.start;
  } else {
    f += StepFrequencyTable[opt.band.step];
  }

  RADIO_TuneTo(f);
}

static void scanFn(bool forward) {
  Loot *msm = &gLoot[gSettings.activeVFO];
  msm->rssi = RADIO_GetRSSI();
  msm->open = isBK1080 ? true : BK4819_IsSquelchOpen();
  LOOT_Update(msm);

  bool rx = msm->open;
  if (gTxState != TX_ON) {
    if (gMonitorMode) {
      rx = true;
    } else if (gNoListen) {
      rx = false;
    } else {
      rx = msm->open;
    }
    RADIO_ToggleRX(rx);
  }
  SP_AddPoint(msm);

  if (newScan) {
    newScan = false;
    startNewScan(false);
  }

  nextF();

  if (gCurrentVFO->fRX == opt.band.bounds.start) {
    startNewScan(false);
    return;
  }
  SP_Next();
}

static void setup(void) {
  centerF = gCurrentVFO->fRX;
  opt.band.step = gCurrentPreset->band.step;
  opt.band.squelch = gCurrentPreset->band.squelch;
  opt.band.bounds.start = centerF - bandwidth / 2;
  opt.band.bounds.end = centerF + bandwidth / 2;
  gSettings.scanTimeout = scanInterval;
}

void ANALYZER_init(void) {
  RADIO_LoadCurrentVFO();
  initialF = gCurrentVFO->fRX;
  initialScanInterval = gSettings.scanTimeout;
  startNewScan(true);
  gRedrawScreen = true;
  gMonitorMode = false;
  gNoListen = true;
  gScanFn = scanFn;
  setup();
  SVC_Toggle(SVC_SCAN, true, 1);
}

void ANALYZER_update(void) {
  if (gIsListening) {
    SP_AddPoint(&gLoot[gSettings.activeVFO]);
  }
}

void ANALYZER_deinit(void) {
  SVC_Toggle(SVC_SCAN, false, 0);
  RADIO_TuneTo(initialF);
  gNoListen = false;
  gSettings.scanTimeout = initialScanInterval;
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
      RADIO_TuneTo(centerF + StepFrequencyTable[opt.band.step]);
      setup();

      startNewScan(true);
      return true;
    case KEY_DOWN:
      RADIO_TuneTo(centerF - StepFrequencyTable[opt.band.step]);
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
      RADIO_TuneToSave(gLastActiveLoot->f);
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

  DrawVLine(spectrumWidth - 1, 8, LCD_HEIGHT - 8, C_FILL);
  SP_Render(&opt, 0, ANALYZER_Y, ANALYZER_HEIGHT);

  for (uint8_t i = ANALYZER_Y; i < ANALYZER_Y + ANALYZER_HEIGHT; i += 4) {
    PutPixel(spectrumWidth / 2 - 1, i, C_FILL);
  }

  LOOT_Sort(LOOT_SortByLastOpenTime, false);

  const uint8_t LOOT_BL = 13;

  for (uint8_t i = 0, ni = 0; ni < 8 && i < LOOT_Size(); i++) {
    Loot *p = LOOT_Item(i);
    if (p->blacklist) {
      continue;
    }

    const uint8_t ybl = ni * 6 + LOOT_BL;
    ni++;

    if (p->open) {
      PrintSmall(spectrumWidth + 1, ybl, ">");
    } else if (p->goodKnown) {
      PrintSmall(spectrumWidth + 1, ybl, "+");
    }

    PrintSmallEx(LCD_WIDTH - 1, ybl, POS_R, C_FILL, "%u.%05u", p->f / 100000,
                 p->f % 100000);
  }

  PrintSmallEx(spectrumWidth - 2, ANALYZER_Y - 3, POS_R, C_FILL, "SQ:%u",
               opt.band.squelch);
  if (gNoListen) {
    PrintSmallEx(0, ANALYZER_Y - 3, POS_L, C_FILL, "No listen");
  }
  PrintSmallEx(0, ANALYZER_Y - 3 + 6, POS_L, C_FILL, "%ums", scanInterval);

  lastRender = elapsedMilliseconds;
}
