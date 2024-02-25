#include "spectrumreborn.h"
#include "../dcs.h"
#include "../driver/st7565.h"
#include "../driver/uart.h"
#include "../helper/lootlist.h"
#include "../helper/presetlist.h"
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
    RADIO_TuneTo(gCurrentPreset->band.bounds.start);
    stepsCount = PRESETS_GetSteps(gCurrentPreset);
    SP_Init(stepsCount, spectrumWidth);
    bandFilled = false;

    gScanRedraw = stepsCount * gSettings.scanTimeout >= 500;
    gScanFn = scanFn;
    uint16_t t = gSettings.scanTimeout < 10 ? gSettings.scanTimeout : 10;
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

  RADIO_NextPresetFreqEx(forward, gSettings.scanTimeout >= 10);

  if (PRESETS_GetChannel(gCurrentPreset, gCurrentVFO->fRX) == stepsCount - 1) {
    scanTime = elapsedMilliseconds - lastReady;
    chPerSec = stepsCount * 1000 / scanTime;
    lastReady = elapsedMilliseconds;
    gRedrawScreen = true;
  }

  if (gCurrentVFO->fRX == gCurrentPreset->band.bounds.start) {
    startNewScan(false);
    return;
  }
  SP_Next();
}

void SPECTRUM_init(void) {
  RADIO_LoadCurrentVFO();
  startNewScan(true);
  gRedrawScreen = true;
  gMonitorMode = false;
  gScanFn = scanFn;
  SVC_Toggle(SVC_SCAN, true, 1);
}

void SPECTRUM_update(void) {
  if (gIsListening) {
    SP_AddPoint(&gLoot[gSettings.activeVFO]);
  }
}

void SPECTRUM_deinit(void) {
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
      PRESETS_SelectPresetRelative(true);
      RADIO_SelectPresetSave(gSettings.activePreset);
      startNewScan(true);
      return true;
    case KEY_DOWN:
      PRESETS_SelectPresetRelative(false);
      RADIO_SelectPresetSave(gSettings.activePreset);
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
      if (gCurrentPreset->band.squelchType == SQUELCH_RSSI) {
        gCurrentPreset->band.squelchType = SQUELCH_RSSI_NOISE_GLITCH;
      } else {
        gCurrentPreset->band.squelchType++;
      }
      startNewScan(true);
      return true;
    case KEY_1:
      if (gSettings.scanTimeout < 255) {
        gSettings.scanTimeout++;
      }
      return true;
    case KEY_7:
      if (gSettings.scanTimeout > 1) {
        gSettings.scanTimeout--;
      }
      return true;
    case KEY_3:
      RADIO_UpdateSquelchLevel(true);
      startNewScan(true);
      return true;
    case KEY_9:
      if (gCurrentPreset->band.squelch > 1) {
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

void SPECTRUM_render(void) {
  Band *band = &gCurrentPreset->band;

  UI_ClearScreen();
  STATUSLINE_SetText(band->name);

  SP_Render(gCurrentPreset, 0, SPECTRUM_Y, SPECTRUM_HEIGHT);

  PrintSmallEx(spectrumWidth - 2, SPECTRUM_Y - 3, POS_R, C_FILL, "SQ%u %s",
               band->squelch, sqTypeNames[band->squelchType]);
  PrintSmallEx(0, SPECTRUM_Y - 3, POS_L, C_FILL, "%ums", gSettings.scanTimeout);
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
