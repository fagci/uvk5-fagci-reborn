#include "vfo1.h"
#include "../helper/lootlist.h"
#include "../helper/rds.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../svc.h"
#include "../svc_scan.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"

static uint32_t lastUpdate = 0;
static DateTime dt;

void VFO1_init(void) {
  RADIO_LoadCurrentVFO();
  gRedrawScreen = true;
}

void VFO1_deinit(void) {}

void VFO1_update(void) {
  if (gIsListening && Now() - lastUpdate >= 1000) {
    gRedrawScreen = true;
    lastUpdate = Now();
  }
}

bool VFO1_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (key == KEY_PTT) {
    RADIO_ToggleTX(bKeyHeld);
    return true;
  }

  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (key) {
    case KEY_UP:
      if (SVC_Running(SVC_SCAN)) {
        gScanForward = true;
        return true;
      }
      RADIO_NextFreqNoClicks(true);
      return true;
    case KEY_DOWN:
      if (SVC_Running(SVC_SCAN)) {
        gScanForward = false;
        return true;
      }
      RADIO_NextFreqNoClicks(false);
      return true;
    default:
      break;
    }
  }

  // long held
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    /* case KEY_2:
      LOOT_Standby();
      RADIO_NextVFO(true);
      msm.f = radio->rx.f;
      return true; */
    case KEY_STAR:
      SVC_Toggle(SVC_SCAN, true, 10);
      return true;
    case KEY_EXIT:
      return true;
    case KEY_3:
      RADIO_ToggleVfoMR();
      return true;
    case KEY_1:
      RADIO_UpdateStep(true);
      return true;
    case KEY_7:
      RADIO_UpdateStep(false);
      return true;
    case KEY_0:
      RADIO_ToggleModulation();
      return true;
    case KEY_6:
      RADIO_ToggleListeningBW();
      return true;
    default:
      break;
    }
  }

  // Simple keypress
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_0:
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
    case KEY_9:
      gFInputCallback = RADIO_TuneToSave;
      APPS_run(APP_FINPUT);
      APPS_key(key, bKeyPressed, bKeyHeld);
      return true;
    case KEY_F:
      APPS_run(APP_VFO_CFG);
      return true;
    case KEY_STAR:
      APPS_run(APP_LOOT_LIST);
      return true;
    case KEY_SIDE1:
      if (SVC_Running(SVC_SCAN)) {
        LOOT_BlacklistLast();
        return true;
      }
      gMonitorMode = !gMonitorMode;
      return true;
    case KEY_SIDE2:
      if (SVC_Running(SVC_SCAN)) {
        LOOT_GoodKnownLast();
        return true;
      }
      break;
    case KEY_EXIT:
      if (SVC_Running(SVC_SCAN)) {
        SVC_Toggle(SVC_SCAN, false, 0);
        return true;
      }
      if (!APPS_exit()) {
        LOOT_Standby();
        RADIO_NextVFO();
      }
      return true;
    default:
      break;
    }
  }
  return false;
}

static void drawRDS() {
  if (rds.RDSSignal) {
    PrintSmallEx(LCD_WIDTH - 1, 12, POS_R, C_FILL, "RDS");

    char genre[17];
    const char wd[8][3] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA", "SU"};
    SI47XX_GetProgramType(genre);
    PrintSmallEx(LCD_XCENTER, 14, POS_C, C_FILL, "%s", genre);

    if (SI47XX_GetLocalDateTime(&dt)) {
      PrintSmallEx(LCD_XCENTER, 22, POS_C, C_FILL,
                   "%02u.%02u.%04u, %s %02u:%02u", dt.day, dt.month, dt.year,
                   wd[dt.wday], dt.hour, dt.minute);
    }

    PrintSmall(0, LCD_HEIGHT - 8, "%s", rds.radioText);
  }
}

void VFO1_render(void) {
  UI_ClearScreen();
  const uint8_t BASE = 38;

  VFO *vfo = &gVFO[gSettings.activeVFO];
  Preset *p = gVFOPresets[gSettings.activeVFO];
  uint32_t f = gTxState == TX_ON ? RADIO_GetTXF() : GetScreenF(vfo->rx.f);

  uint16_t fp1 = f / 100000;
  uint16_t fp2 = f / 100 % 1000;
  uint8_t fp3 = f % 100;
  const char *mod =
      modulationTypeOptions[vfo->modulation == MOD_PRST ? p->band.modulation
                                                        : vfo->modulation];
  if (gIsListening) {
    UI_RSSIBar(gLoot[gSettings.activeVFO].rssi, vfo->rx.f, BASE + 2);
  }

  if (radio->channel >= 0) {
    PrintMediumEx(LCD_XCENTER, BASE - 16, POS_C, C_FILL,
                  gVFONames[gSettings.activeVFO]);
  }

  if (gTxState && gTxState != TX_ON) {
    PrintMediumBoldEx(LCD_XCENTER, BASE, POS_C, C_FILL, "%s",
                      TX_STATE_NAMES[gTxState]);
  } else {
    PrintBiggestDigitsEx(LCD_WIDTH - 22, BASE, POS_R, C_FILL, "%4u.%03u", fp1,
                         fp2);
    PrintBigDigitsEx(LCD_WIDTH - 1, BASE, POS_R, C_FILL, "%02u", fp3);
    PrintMediumEx(LCD_WIDTH - 1, BASE - 12, POS_R, C_FILL, mod);
  }

  drawRDS();
}
