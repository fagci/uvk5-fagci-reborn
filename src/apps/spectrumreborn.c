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

static uint8_t spectrumWidth = 84;

static bool newScan = false;
static bool noListen = false;
static bool bandFilled = false;

static uint32_t lastRender = 0;
static uint32_t lastUpdate = 0;

static void startNewScan(bool reset) {
  if (reset) {
    LOOT_Standby();
    RADIO_TuneTo(gCurrentPreset->band.bounds.start);
    lastUpdate = elapsedMilliseconds;
    SP_Init(PRESETS_GetSteps(gCurrentPreset), spectrumWidth);
    bandFilled = false;
  } else {
    SP_Begin();
    bandFilled = true;
  }
}

static void scanFn(bool forward) {
  RADIO_UpdateMeasurements();

  Loot *msm = &gLoot[gSettings.activeVFO];

  LOOT_Update(msm);

  SP_AddPoint(msm);

  if (newScan) {
    newScan = false;
    startNewScan(false);
  }

  Loot *loot = LOOT_Get(msm->f);
  if (gIsListening && (loot && !loot->blacklist)) {
    return;
  }

  RADIO_NextPresetFreq(true);
  lastUpdate = elapsedMilliseconds;

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
  gScanFn = scanFn;
  SVC_Toggle(SVC_SCAN, true, 10);
}

void SPECTRUM_update(void) {
  if (elapsedMilliseconds - lastRender >= 500) {
    lastRender = elapsedMilliseconds;
    gRedrawScreen = true;
  }
}

void SPECTRUM_deinit(void) { SVC_Toggle(SVC_SCAN, false, 0); }

bool SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    if (Key == KEY_SIDE1) {
      noListen = !noListen;
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
    case KEY_3:
      RADIO_UpdateSquelchLevel(true);
      startNewScan(true);
      return true;
    case KEY_9:
      if (gCurrentPreset->band.squelch > 1) {
        RADIO_UpdateSquelchLevel(false);
        startNewScan(true);
      }
      return true;
    case KEY_PTT:
      RADIO_TuneToSave(gLastActiveLoot->f);
      APPS_run(APP_STILL);
      return true;
    case KEY_1:
      UART_ToggleLog(true);
      return true;
    case KEY_7:
      UART_ToggleLog(false);
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

  DrawVLine(spectrumWidth - 1, 8, LCD_HEIGHT - 8, C_FILL);
  SP_Render(gCurrentPreset, 0, SPECTRUM_Y, SPECTRUM_HEIGHT);

  LOOT_Sort(LOOT_SortByLastOpenTime, false);

  const uint8_t LOOT_BL = 13;
  Log("[SPECTRUM] Loot size: %u", LOOT_Size());

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

  PrintSmallEx(spectrumWidth - 2, SPECTRUM_Y - 3, POS_R, C_FILL, "SQ:%u",
               gCurrentPreset->band.squelch);
  if (noListen) {
    PrintSmallEx(0, SPECTRUM_Y - 3, POS_L, C_FILL, "No listen");
  }

  lastRender = elapsedMilliseconds;
}
