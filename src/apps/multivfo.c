#include "multivfo.h"
#include "../dcs.h"
#include "../helper/bandlist.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
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

static AppVFOSlots slots = {.maxCount = 2};

static void tuneTo(uint32_t f) { RADIO_TuneToSave(GetTuneF(f)); }

void MULTIVFO_init() {
  RADIO_LoadCurrentCH();

  gRedrawScreen = true;
}

void MULTIVFO_deinit() {}

void MULTIVFO_update() {
  if (elapsedMilliseconds - lastRender >= 500) {
    gRedrawScreen = true;
    lastRender = elapsedMilliseconds;
  }
}

static void setChannel(uint16_t v) { RADIO_TuneToCH(v - 1); }

bool MULTIVFO_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
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
    OffsetDirection offsetDirection = radio->offsetDir;
    switch (key) {
    case KEY_EXIT:
      return true;
    case KEY_1:
      APPS_run(APP_BANDS_LIST);
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
      radio->offsetDir = offsetDirection;
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
      APPS_run(APP_CH_CFG);
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

static void render2CHPart(uint8_t i) {
  const uint8_t BASE = 22;
  const uint8_t bl = BASE + 34 * i;

  Band *p = gCHBands[i];
  CH *vfo = &gCH[i];
  const bool isActive = gSettings.activeCH == i;
  const Loot *loot = &gLoot[i];

  uint32_t f =
      gTxState == TX_ON && isActive ? RADIO_GetTXF() : GetScreenF(vfo->f);

  const uint16_t fp1 = f / 100000;
  const uint16_t fp2 = f / 100 % 1000;
  const uint8_t fp3 = f % 100;
  const char *mod = modulationTypeOptions[pmodulation];

  if (isActive && gTxState <= TX_ON) {
    FillRect(0, bl - 14, 28, 7, C_FILL);
    if (gTxState == TX_ON) {
      PrintMediumEx(0, bl, POS_L, C_INVERT, "TX");
    }
    if (gIsListening) {
      PrintMediumEx(0, bl, POS_L, C_INVERT, "RX");
      if (!gIsBK1080) {
        UI_RSSIBar(gLoot[i].rssi, vfo->f, 31);
      }
    }
  }

  if (gTxState && gTxState != TX_ON && isActive) {
    PrintMediumBoldEx(LCD_XCENTER, bl - 8, POS_C, C_FILL, "%s",
                      TX_STATE_NAMES[gTxState]);
    PrintSmallEx(LCD_XCENTER, bl - 8 + 6, POS_C, C_FILL, "%u", RADIO_GetTXF());
  } else {
    if (vfo->channel >= 0) {
      PrintMediumBoldEx(LCD_XCENTER, bl - 8, POS_C, C_FILL, gCHNames[i]);
      PrintMediumEx(LCD_XCENTER, bl, POS_C, C_FILL, "%4u.%03u", fp1, fp2);
      PrintSmallEx(14, bl - 9, POS_C, C_INVERT, "MR %03u", vfo->channel + 1);
    } else {
      PrintBigDigitsEx(LCD_WIDTH - 19, bl, POS_R, C_FILL, "%4u.%03u", fp1, fp2);
      PrintMediumBoldEx(LCD_WIDTH, bl, POS_R, C_FILL, "%02u", fp3);
      PrintSmallEx(14, bl - 9, POS_C, C_INVERT, "CH");
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
               pallowTx ? TX_POWER_NAMES[ppower][0] : ' ', "WNn"[pbw],
               p -> band.squelch,
               RADIO_GetTXFEx(vfo, p) != vfo->f
                   ? (poffsetDir ? TX_OFFSET_NAMES[poffsetDir][0] : '*')
                   : ' ');

  if (loot->lastTimeOpen) {
    PrintSmallEx(LCD_WIDTH, bl + 6, POS_R, C_FILL, "%02u:%02u %us", est / 60,
                 est % 60, loot->duration / 1000);
  } else {
    PrintSmallEx(LCD_WIDTH, bl + 6, POS_R, C_FILL, "%d.%02dk",
                 StepFrequencyTable[p->step] / 100,
                 StepFrequencyTable[p->step] % 100);
  }
}

void CH1_render() {
  UI_ClearScreen();
  const uint8_t BASE = 38;

  CH *vfo = &gCH[gSettings.activeCH];
  uint32_t f = gTxState == TX_ON ? RADIO_GetTXF() : GetScreenF(vfo->f);

  uint16_t fp1 = f / 100000;
  uint16_t fp2 = f / 100 % 1000;
  uint8_t fp3 = f % 100;
  const char *mod = modulationTypeOptions[vfo->modulation];
  if (gIsListening) {
    if (!gIsBK1080) {
      UI_RSSIBar(gLoot[gSettings.activeCH].rssi, vfo->f, BASE + 2);
    }
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
}

void MULTIVFO_render() {
  UI_ClearScreen();

  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  } else {
    STATUSLINE_SetText("%s:%u", gCurrentBand->name,
                       BANDS_GetChannel(gCurrentBand, radio->f) + 1);
  }

  render2CHPart(0);
  render2CHPart(1);
}

static App meta = {
    .id = APP_MULTIVFO,
    .name = "MULTIVFO",
    .runnable = true,
    .init = MULTIVFO_init,
    .update = MULTIVFO_update,
    .render = MULTIVFO_render,
    .key = MULTIVFO_key,
    .deinit = MULTIVFO_deinit,
};

App *MULTIVFO_Meta() { return &meta; }
