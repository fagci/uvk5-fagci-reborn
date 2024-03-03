#include "vfo2.h"
#include "../dcs.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../helper/presetlist.h"
#include "../scheduler.h"
#include "../settings.h"
#include "../svc.h"
#include "../svc_scan.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "finput.h"

static uint32_t lastRender = 0;

static void tuneTo(uint32_t f) { RADIO_TuneToSave(GetTuneF(f)); }

void VFO2_init(void) {
  RADIO_LoadCurrentVFO();

  gRedrawScreen = true;
}

void VFO2_deinit(void) {}

void VFO2_update(void) {
  if (elapsedMilliseconds - lastRender >= 500) {
    gRedrawScreen = true;
    lastRender = elapsedMilliseconds;
  }
}

static void setChannel(uint16_t v) { RADIO_TuneToCH(v - 1); }

bool VFO2_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld && radio->channel >= 0) {
    if (!gIsNumNavInput && key <= KEY_9) {
      NUMNAV_Init(radio->channel + 1, 1, CHANNELS_GetCountMax());
      gNumNavCallback = setChannel;
    }
    if (gIsNumNavInput) {
      NUMNAV_Input(key);
      return true;
    }
  }
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
      }
      RADIO_NextFreq(true);
      return true;
    case KEY_DOWN:
      if (SVC_Running(SVC_SCAN)) {
        gScanForward = false;
      }
      RADIO_NextFreq(false);
      return true;
    default:
      break;
    }
  }

  // long held
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    OffsetDirection offsetDirection = gCurrentPreset->offsetDir;
    switch (key) {
    case KEY_EXIT:
      return true;
    case KEY_1:
      APPS_run(APP_PRESETS_LIST);
      return true;
    case KEY_2:
      LOOT_Standby();
      RADIO_NextVFO();
      return true;
    case KEY_3:
      RADIO_ToggleVfoMR();
      return true;
    case KEY_4: // freq catch
      return true;
    case KEY_5: // noaa
      return true;
    case KEY_6:
      RADIO_ToggleTxPower();
      return true;
    case KEY_7:
      RADIO_UpdateStep(true);
      return true;
    case KEY_8:
      IncDec8(&offsetDirection, 0, OFFSET_MINUS, 1);
      gCurrentPreset->offsetDir = offsetDirection;
      return true;
    case KEY_9: // call
      return true;
    case KEY_0:
      RADIO_ToggleModulation();
      return true;
    case KEY_STAR:
      SVC_Toggle(SVC_SCAN, true, 10);
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
      gFInputCallback = tuneTo;
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

static void render2VFOPart(uint8_t i) {
  const uint8_t BASE = 22;
  const uint8_t bl = BASE + 34 * i;

  Preset *p = gVFOPresets[i];
  VFO *vfo = &gVFO[i];
  const bool isActive = gSettings.activeVFO == i;
  const Loot *loot = &gLoot[i];

  uint32_t f =
      gTxState == TX_ON && isActive ? RADIO_GetTXF() : GetScreenF(vfo->rx.f);

  const uint16_t fp1 = f / 100000;
  const uint16_t fp2 = f / 100 % 1000;
  const uint8_t fp3 = f % 100;
  const char *mod = modulationTypeOptions[p->band.modulation];

  if (isActive && gTxState <= TX_ON) {
    FillRect(0, bl - 14, 28, 7, C_FILL);
    if (gTxState == TX_ON) {
      PrintMediumEx(0, bl, POS_L, C_INVERT, "TX");
    }
    if (gIsListening) {
      PrintMediumEx(0, bl, POS_L, C_INVERT, "RX");
      if (!isBK1080) {
        UI_RSSIBar(gLoot[i].rssi, vfo->rx.f, 31);
      }
    }
  }

  if (gTxState && gTxState != TX_ON && isActive) {
    PrintMediumBoldEx(LCD_XCENTER, bl - 8, POS_C, C_FILL, "%s",
                      TX_STATE_NAMES[gTxState]);
    PrintSmallEx(LCD_XCENTER, bl - 8 + 6, POS_C, C_FILL, "%u", RADIO_GetTXF());
  } else {
    if (vfo->channel >= 0) {
      PrintMediumBoldEx(LCD_XCENTER, bl - 8, POS_C, C_FILL, gVFONames[i]);
      PrintMediumEx(LCD_XCENTER, bl, POS_C, C_FILL, "%4u.%03u", fp1, fp2);
      PrintSmallEx(14, bl - 9, POS_C, C_INVERT, "MR %03u", vfo->channel + 1);
    } else {
      PrintBigDigitsEx(LCD_WIDTH - 19, bl, POS_R, C_FILL, "%4u.%03u", fp1, fp2);
      PrintMediumBoldEx(LCD_WIDTH, bl, POS_R, C_FILL, "%02u", fp3);
      PrintSmallEx(14, bl - 9, POS_C, C_INVERT, "VFO");
    }
    PrintSmallEx(LCD_WIDTH, bl - 9, POS_R, C_FILL, mod);
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
  PrintSmallEx(LCD_XCENTER, bl + 6, POS_C, C_FILL, "%c %c SQ%u %c",
               p->allowTx ? TX_POWER_NAMES[p->power][0] : ' ',
               "WNn"[p->band.bw], p -> band.squelch,
               RADIO_GetTXFEx(vfo, p) != vfo->rx.f
                   ? (p->offsetDir ? TX_OFFSET_NAMES[p->offsetDir][0] : '*')
                   : ' ');

  if (loot->lastTimeOpen) {
    PrintSmallEx(LCD_WIDTH, bl + 6, POS_R, C_FILL, "%02u:%02u %us", est / 60,
                 est % 60, loot->duration / 1000);
  } else {
    PrintSmallEx(LCD_WIDTH, bl + 6, POS_R, C_FILL, "%d.%02dk",
                 StepFrequencyTable[p->band.step] / 100,
                 StepFrequencyTable[p->band.step] % 100);
  }
}

void VFO2_render(void) {
  UI_ClearScreen();

  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  } else {
    STATUSLINE_SetText("%s:%u", gCurrentPreset->band.name,
                       PRESETS_GetChannel(gCurrentPreset, radio->rx.f) + 1);
  }

  render2VFOPart(0);
  render2VFOPart(1);
}

static VFO vfo;

static App meta = {
    .id = APP_VFO2,
    .name = "VFO2",
    .runnable = true,
    .init = VFO2_init,
    .update = VFO2_update,
    .render = VFO2_render,
    .key = VFO2_key,
    .deinit = VFO2_deinit,
    .vfo = &vfo,
};

App *VFO2_Meta(void) { return &meta; }
