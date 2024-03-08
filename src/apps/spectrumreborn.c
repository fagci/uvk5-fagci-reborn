// #include "spectrumreborn.h"
#include "../dcs.h"
#include "../driver/st7565.h"
#include "../driver/uart.h"
#include "../helper/lootlist.h"
#include "../helper/bandlist.h"
#include "../scheduler.h"
#include "../settings.h"
#include "../svc.h"
#include "../svc_scan.h"
#include "../ui/graphics.h"
#include "../ui/spectrum.h"
#include "../ui/statusline.h"
#include "apps.h"

static const uint8_t SPECTRUM_Y = 16;
static const uint8_t SPECTRUM_HEIGHT = 40;

static uint8_t spectrumWidth = LCD_WIDTH;

static bool newScan = false;
static bool bandFilled = false;

static uint32_t lastRender = 0;
static uint32_t stepsCount = 0;

static uint32_t lastReady = 0;
static uint32_t chPerSec = 0;
static uint32_t scanTime = 0;


static void scanFn(bool forward);

static void startNewScan(bool reset) {
  if (reset) {
    SVC_Toggle(SVC_SCAN, false, 0);
    SVC_Toggle(SVC_LISTEN, false, 10);
    LOOT_Standby();
    RADIO_TuneTo(gCurrentBand->band.bounds.start);
    stepsCount = BANDS_GetSteps(gCurrentBand);
    SP_Init(stepsCount, spectrumWidth);
    bandFilled = false;

    gScanRedraw = stepsCount * vfo.scan.timeout >= 500;
    gScanFn = scanFn;
    uint16_t t = vfo.scan.timeout < 10 ? vfo.scan.timeout : 10;
    lastReady = elapsedMilliseconds;
    SVC_Toggle(SVC_SCAN, true, t);
    SVC_Toggle(SVC_LISTEN, true, t);

  } else {
    SP_Begin();
    bandFilled = true;
  }
}

static void scanFn(bool forward) {
  SP_AddPoint(RADIO_UpdateMeasurements());

  if (newScan) {
    newScan = false;
    startNewScan(false);
  }

  RADIO_NextBandFreqEx(forward, vfo.scan.timeout >= 10);

  if (BANDS_GetChannel(gCurrentBand, radio->f) == stepsCount - 1) {
    scanTime = elapsedMilliseconds - lastReady;
    chPerSec = stepsCount * 1000 / scanTime;
    lastReady = elapsedMilliseconds;
    gRedrawScreen = true;
  }

  if (radio->f == gCurrentBand->band.bounds.start) {
    startNewScan(false);
    return;
  }
  SP_Next();
}

void SPECTRUM_init() {
  RADIO_LoadCurrentCH();
  startNewScan(true);
  gRedrawScreen = true;
  gMonitorMode = false;
  gScanFn = scanFn;
  SVC_Toggle(SVC_SCAN, true, 1);
}

void SPECTRUM_update() {
  if (gIsListening) {
    SP_AddPoint(&gLoot[gSettings.activeCH]);
  }
}

void SPECTRUM_deinit() {
  SVC_Toggle(SVC_SCAN, false, 0);
  SVC_Toggle(SVC_LISTEN, false, 0);
  SVC_Toggle(SVC_LISTEN, true, 10);
}

bool SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    if (Key == KEY_SIDE1) {
      gSettings.noListen = !gSettings.noListen;
      SETTINGS_Save();
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
      BANDS_SelectBandRelative(true);
      RADIO_SelectBandSave(gSettings.activeBand);
      startNewScan(true);
      return true;
    case KEY_DOWN:
      BANDS_SelectBandRelative(false);
      RADIO_SelectBandSave(gSettings.activeBand);
      startNewScan(true);
      return true;
    case KEY_SIDE1:
      LOOT_BlacklistLast();
      return true;
    case KEY_SIDE2:
      LOOT_GoodKnownLast();
      return true;
    case KEY_F:
      APPS_run(APP_BAND_CFG);
      return true;
    case KEY_0:
      APPS_run(APP_BANDS_LIST);
      return true;
    case KEY_STAR:
      APPS_run(APP_LOOT_LIST);
      return true;
    case KEY_5:
      if (radio->sq.levelType == SQUELCH_RSSI) {
        radio->sq.levelType = SQUELCH_RSSI_NOISE_GLITCH;
      } else {
        radio->sq.levelType++;
      }
      startNewScan(true);
      return true;
    case KEY_1:
      if (vfo.scan.timeout < 255) {
        vfo.scan.timeout++;
      }
      return true;
    case KEY_7:
      if (vfo.scan.timeout > 1) {
        vfo.scan.timeout--;
      }
      return true;
    case KEY_3:
      RADIO_UpdateSquelchLevel(true);
      startNewScan(true);
      return true;
    case KEY_9:
      if (radio->sq.level > 1) {
        RADIO_UpdateSquelchLevel(false);
      }
      startNewScan(true);
      return true;
    case KEY_PTT:
      RADIO_TuneToSave(gLastActiveLoot->f);
      APPS_run(APP_STILL);
      return true;
    default:
      break;
    }
  }
  return false;
}

void SPECTRUM_render() {
  Band *band = &gCurrentBand->band;

  UI_ClearScreen();
  STATUSLINE_SetText(band->name);

  SP_Render(gCurrentBand, 0, SPECTRUM_Y, SPECTRUM_HEIGHT);

  PrintSmallEx(spectrumWidth - 2, SPECTRUM_Y - 3, POS_R, C_FILL, "SQ%u %s",
               band->squelch, SQ_TYPE_NAMES[band->squelchType]);
  PrintSmallEx(0, SPECTRUM_Y - 3, POS_L, C_FILL, "%ums", vfo.scan.timeout);
  PrintSmallEx(0, SPECTRUM_Y - 3 + 6, POS_L, C_FILL, "%ums", scanTime);
  PrintSmallEx(0, SPECTRUM_Y - 3 + 12, POS_L, C_FILL, "%uCHps", chPerSec);

  uint32_t fs = band->bounds.start;
  uint32_t fe = band->bounds.end;

  PrintSmallEx(0, LCD_HEIGHT - 1, POS_L, C_FILL, "%u.%05u", fs / 100000,
               fs % 100000);
  PrintSmallEx(LCD_WIDTH, LCD_HEIGHT - 1, POS_R, C_FILL, "%u.%05u", fe / 100000,
               fe % 100000);

  if (gLastActiveLoot) {
    PrintMediumBoldEx(LCD_XCENTER, 16, POS_C, C_FILL, "%u.%05u",
                      gLastActiveLoot->f / 100000, gLastActiveLoot->f % 100000);
    if (gLastActiveLoot->ct != 0xFF) {
      PrintSmallEx(LCD_XCENTER, 16 + 6, POS_C, C_FILL, "CT:%u.%uHz",
                   CTCSS_Options[gLastActiveLoot->ct] / 10,
                   CTCSS_Options[gLastActiveLoot->ct] % 10);
    }
  }

  if (band->squelchType == SQUELCH_RSSI) {
    uint8_t b = band->bounds.start > SETTINGS_GetFilterBound() ? 1 : 0;
    SP_RenderRssi(SQ[b][0][band->squelch], "", true, 0, SPECTRUM_Y,
                  SPECTRUM_HEIGHT);
  }

  lastRender = elapsedMilliseconds;
}

static App meta = {
    .id = APP_SPECTRUM,
    .name = "Spectrum band",
    .runnable = true,
    .init = SPECTRUM_init,
    .update = SPECTRUM_update,
    .render = SPECTRUM_render,
    .key = SPECTRUM_key,
    .deinit = SPECTRUM_deinit,
};

App *SPECTRUM_Meta() { return &meta; }
