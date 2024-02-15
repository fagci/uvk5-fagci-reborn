#include "spectrumreborn.h"
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

static void startNewScan(bool reset) {
  if (reset) {
    LOOT_Standby();
    RADIO_TuneTo(gCurrentPreset->band.bounds.start);
    SP_Init(PRESETS_GetSteps(gCurrentPreset), spectrumWidth);
    bandFilled = false;
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

  RADIO_NextPresetFreq(forward);

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
  gNoListen = true;
  gScanFn = scanFn;
  SVC_Toggle(SVC_SCAN, true, 10);
}

void SPECTRUM_update(void) {
  if (gIsListening) {
    SP_AddPoint(&gLoot[gSettings.activeVFO]);
  }
}

void SPECTRUM_deinit(void) {
  SVC_Toggle(SVC_SCAN, false, 0);
  gNoListen = false;
}

bool SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
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
      PRESETS_SelectPresetRelative(true);
      startNewScan(true);
      return true;
    case KEY_DOWN:
      PRESETS_SelectPresetRelative(false);
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
      return true;
    case KEY_9:
      if (gCurrentPreset->band.squelch > 1) {
        RADIO_UpdateSquelchLevel(false);
      }
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
  UI_ClearScreen();
  STATUSLINE_SetText(gCurrentPreset->band.name);

  SP_Render(gCurrentPreset, 0, SPECTRUM_Y, SPECTRUM_HEIGHT);

  PrintSmallEx(spectrumWidth - 2, SPECTRUM_Y - 3, POS_R, C_FILL, "SQ:%u",
               gCurrentPreset->band.squelch);
  PrintSmallEx(0, SPECTRUM_Y - 3, POS_L, C_FILL, "%ums", gSettings.scanTimeout);

  uint32_t fs = gCurrentPreset->band.bounds.start;
  uint32_t fe = gCurrentPreset->band.bounds.end;

  PrintSmallEx(0, LCD_HEIGHT - 1, POS_L, C_FILL, "%u.%05u", fs / 100000,
               fs % 100000);
  PrintSmallEx(LCD_WIDTH, LCD_HEIGHT - 1, POS_R, C_FILL, "%u.%05u", fe / 100000,
               fe % 100000);

  if (gLastActiveLoot) {
    PrintMediumBoldEx(LCD_XCENTER, 16, POS_C, C_FILL, "%u.%05u",
                      gLastActiveLoot->f / 100000, gLastActiveLoot->f % 100000);
  }

  lastRender = elapsedMilliseconds;
}
