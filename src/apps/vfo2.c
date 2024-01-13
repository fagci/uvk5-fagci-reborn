#include "vfo2.h"
#include "../dcs.h"
#include "../driver/uart.h"
#include "../helper/adapter.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"

static uint32_t lastRender = 0;
static uint32_t lastUpdate = 0;

// TODO: check if any msm/loot is buggy

void VFO2_init() {
  RADIO_SetupByCurrentVFO(); // TODO: reread from EEPROM not needed maybe

  gRedrawScreen = true;
}

void VFO2_deinit() {
  /* if (APPS_Peek() != APP_FINPUT && APPS_Peek() != APP_VFO2) {
    RADIO_ToggleRX(false);
  } */
}

void VFO2_update() {
  if (elapsedMilliseconds - lastUpdate >= (isBK1080 ? 1000 : 10)) {
    RADIO_UpdateMeasurements();
    lastUpdate = elapsedMilliseconds;

    if (elapsedMilliseconds - lastRender >= 500) {
      gRedrawScreen = true;
      lastRender = elapsedMilliseconds;
    }
  }
}

bool VFO2_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (key == KEY_PTT) {
    RADIO_ToggleTX(bKeyPressed);
    return true;
  }

  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (key) {
    case KEY_UP:
      RADIO_NextFreq(true);
      return true;
    case KEY_DOWN:
      RADIO_NextFreq(false);
      return true;
    default:
      break;
    }
  }

  // long held
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    case KEY_2:
      LOOT_Standby();
      RADIO_NextVFO(true);
      RADIO_TuneToPure(gCurrentVFO->fRX);
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
    case KEY_SIDE1:
      gMonitorMode = !gMonitorMode;
      return true;
    case KEY_EXIT:
      if (!APPS_exit()) {
        LOOT_Standby();
        RADIO_NextVFO(true);
        RADIO_TuneToPure(gCurrentVFO->fRX);
      }
      return true;
    default:
      break;
    }
  }
  return false;
}

static void render2VFOPart(uint8_t i) {
  const uint8_t BASE = 22;
  const uint8_t bl = BASE + 34 * i;

  Preset *p = gVFOPresets[i];
  const VFO *vfo = &gVFO[i];
  const bool isActive = gSettings.activeVFO == i;
  const Loot *loot = &gLoot[i];

  const uint16_t fp1 = vfo->fRX / 100000;
  const uint16_t fp2 = vfo->fRX / 100 % 1000;
  const uint8_t fp3 = vfo->fRX % 100;
  const char *mod = modulationTypeOptions[p->band.modulation];

  if (isActive) {
    FillRect(0, bl - 14, 28, 7, C_FILL);
    if (gTxState == TX_ON) {
      PrintMediumEx(0, bl, POS_L, C_INVERT, "TX");
    }
    if (gIsListening) {
      PrintMediumEx(0, bl, POS_L, C_INVERT, "RX");
      UI_RSSIBar(gLoot[i].rssi, vfo->fRX, 31);
    }
  }

  if (gTxState && gTxState != TX_ON && isActive) {
    PrintMediumBoldEx(LCD_XCENTER, bl - 8, POS_C, C_FILL, "%s",
                      TX_STATE_NAMES[gTxState]);
  } else {
    if (vfo->isMrMode) {
      PrintMediumBoldEx(LCD_XCENTER, bl - 8, POS_C, C_FILL, gVFONames[i]);
      PrintMediumEx(LCD_XCENTER, bl, POS_C, C_FILL, "%4u.%03u", fp1, fp2);
      PrintSmallEx(14, bl - 9, POS_C, C_INVERT, "MR %03u", vfo->channel + 1);
    } else {
      PrintBigDigitsEx(LCD_WIDTH - 19, bl, POS_R, C_FILL, "%4u.%03u", fp1, fp2);
      PrintMediumBoldEx(LCD_WIDTH - 1, bl, POS_R, C_FILL, "%02u", fp3);
      PrintSmallEx(14, bl - 9, POS_C, C_INVERT, "VFO");
    }
    PrintSmallEx(LCD_WIDTH - 1, bl - 9, POS_R, C_FILL, mod);
  }

  uint32_t est = loot->lastTimeOpen
                     ? (elapsedMilliseconds - loot->lastTimeOpen) / 1000
                     : 0;
  if (loot->ct != 0xFF) {
    PrintSmallEx(0, bl + 6, POS_L, C_FILL, "CT:%u.%uHz",
                 CTCSS_Options[loot->ct] / 10, CTCSS_Options[loot->ct] % 10);
  } else if (loot->cd != 0xFF) {
    PrintSmallEx(0, bl + 6, POS_L, C_FILL, "D%03oN(fake)",
                 DCS_Options[loot->cd]);
  }
  PrintSmallEx(LCD_XCENTER, bl + 6, POS_C, C_FILL, "%s (#%u)", p->band.name,
               PRESETS_GetChannel(p, vfo->fRX) + 1);
  PrintSmallEx(LCD_WIDTH - 1, bl + 6, POS_R, C_FILL, "%02u:%02u %us", est / 60,
               est % 60, loot->duration / 1000);
}

#include "../ui/statusline.h"

void VFO2_render() {
  UI_ClearScreen();
  STATUSLINE_SetText("%u", gCurrentVFO->fRX);

  render2VFOPart(0);
  render2VFOPart(1);
}
