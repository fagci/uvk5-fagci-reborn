#include "vfo1.h"
#include "../apps/textinput.h"
#include "../driver/bk4819.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/numnav.h"
#include "../helper/presetlist.h"
#include "../helper/rds.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../svc.h"
#include "../svc_render.h"
#include "../svc_scan.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"

static DateTime dt;
static void setChannel(uint16_t v) { RADIO_TuneToCH(v - 1); }
static void tuneTo(uint32_t f) { RADIO_TuneToSave(GetTuneF(f)); }

static bool SSB_Seek_ON = false;
static bool SSB_Seek_UP = true;

static char message[16] = {'\0'};
static void sendDtmf() {
  RADIO_ToggleTX(true);
  if (gTxState == TX_ON) {
    BK4819_EnterDTMF_TX(true);
    BK4819_PlayDTMFString(message, true, 100, 100, 100, 100);
    RADIO_ToggleTX(false);
  }
}

void VFO1_init(void) { RADIO_LoadCurrentVFO(); }

void VFO1_update(void) {

if (SSB_Seek_ON) {
  if (RADIO_GetRadio() == RADIO_SI4732 && RADIO_IsSSB()) {
   if (Now() - gLastRender  >= 250) {
         if (SSB_Seek_UP) {
            gScanForward = true;
            RADIO_NextFreqNoClicks(true);
         } else {
            gScanForward = false;
            RADIO_NextFreqNoClicks(false);
         }
        gRedrawScreen = true;
      }
  }
}

  if (gIsListening && Now() - gLastRender >= 500) {
    gRedrawScreen = true;
  }
}

bool VFO1_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
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
  if (key == KEY_PTT && !gIsNumNavInput) {
    RADIO_ToggleTX(bKeyHeld);
    return true;
  }

  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    bool isSsb = RADIO_IsSSB();
    switch (key) {
    case KEY_UP:
      SSB_Seek_ON=false; 
      SSB_Seek_UP=true;   
      if (SVC_Running(SVC_SCAN)) {
        gScanForward = true;
      }
      RADIO_NextFreqNoClicks(true);
      return true;
    case KEY_DOWN:
      SSB_Seek_ON=false;
      SSB_Seek_UP=false;
      if (SVC_Running(SVC_SCAN)) {
        gScanForward = false;
      }
      RADIO_NextFreqNoClicks(false);
      return true;
    case KEY_SIDE1:
      if (RADIO_GetRadio() == RADIO_SI4732 && isSsb) {
        RADIO_TuneToSave(radio->rx.f + 5);
        return true;
      }
      break;
    case KEY_SIDE2:
      if (RADIO_GetRadio() == RADIO_SI4732 && isSsb) {
        RADIO_TuneToSave(radio->rx.f - 5);
        return true;
      }
      break;
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
    case KEY_3:
      RADIO_ToggleVfoMR();
      return true;
    case KEY_4: // freq catch
      SVC_Toggle(SVC_FC, !SVC_Running(SVC_FC), 100);
      return true;
    case KEY_5: // noaa
      SVC_Toggle(SVC_BEACON, !SVC_Running(SVC_BEACON), 15000);
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
      gTextInputSize = 15;
      gTextinputText = message;
      gTextInputCallback = sendDtmf;
      APPS_run(APP_TEXTINPUT);
      return true;
    case KEY_0:
      RADIO_ToggleModulation();
      return true;
    case KEY_STAR:
      if (RADIO_GetRadio() == RADIO_SI4732 && RADIO_IsSSB()) {
        SSB_Seek_ON=true;
        // todo: scan by snr
      } else {
        SVC_Toggle(SVC_SCAN, true, 10);
      }
      return true;
    case KEY_SIDE1:
      APPS_run(APP_ANALYZER);
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
      if (SVC_Running(SVC_SCAN)) {
        if (radio->channel == -1) {
          RADIO_SelectPresetSave(key + 5);
        } else {
          // todo: switch scanlist
        }
      } else {
        gFInputCallback = tuneTo;
        APPS_run(APP_FINPUT);
        APPS_key(key, bKeyPressed, bKeyHeld);
      }
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
      PrintSmallEx(LCD_XCENTER, LCD_HEIGHT - 16, POS_C, C_FILL,
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
