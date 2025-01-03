#include "vfo1.h"
#include "../apps/textinput.h"
#include "../driver/bk4819.h"
#include "../driver/uart.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../helper/scan.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../svc.h"
#include "../svc_render.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "chcfg.h"
#include "chlist.h"
#include "finput.h"

bool gVfo1ProMode = false;

static uint8_t menuIndex = 0;
static bool registerActive = false;

static char String[16];

static const RegisterSpec registerSpecs[] = {
    {"Gain", BK4819_REG_13, 0, 0xFFFF, 1},
    /* {"RF", BK4819_REG_43, 12, 0b111, 1},
    {"RFwe", BK4819_REG_43, 9, 0b111, 1}, */

    // {"IF", 0x3D, 0, 0xFFFF, 100},

    {"DEV", 0x40, 0, 0xFFF, 10},
    // {"300T", 0x44, 0, 0xFFFF, 1000},
    RS_RF_FILT_BW,
    // {"AFTxfl", 0x43, 6, 0b111, 1}, // 7 is widest
    // {"3kAFrsp", 0x74, 0, 0xFFFF, 100},
    {"CMP", 0x31, 3, 1, 1},
    {"MIC", 0x7D, 0, 0xF, 1},

    {"AGCL", 0x49, 0, 0b1111111, 1},
    {"AGCH", 0x49, 7, 0b1111111, 1},
    {"AFC", 0x73, 0, 0xFF, 1},
};

static void UpdateRegMenuValue(RegisterSpec s, bool add) {
  uint16_t v, maxValue;

  if (s.num == BK4819_REG_13) {
    v = radio->gainIndex;
    maxValue = ARRAY_SIZE(gainTable) - 1;
    Log("GAIN v=%u, max=%u", v, maxValue);
  } else if (s.num == 0x73) {
    v = BK4819_GetAFC();
    maxValue = 8;
  } else {
    v = BK4819_GetRegValue(s);
    maxValue = s.mask;
  }

  if (add && v <= maxValue - s.inc) {
    v += s.inc;
  } else if (!add && v >= 0 + s.inc) {
    v -= s.inc;
  }
  Log("GAIN v=%u, max=%u", v, maxValue);

  if (s.num == BK4819_REG_13) {
    RADIO_SetGain(v);
    RADIO_SaveCurrentVFO();
  } else if (s.num == 0x73) {
    BK4819_SetAFC(v);
  } else {
    if (s.num == BK4819_REG_40) {
      gSettings.deviation = v * 10;
      SETTINGS_Save();
    }
    BK4819_SetRegValue(s, v);
  }
}

static void setChannel(uint16_t v) { RADIO_TuneToMR(v - 1); }

static void tuneTo(uint32_t f) {
  RADIO_TuneToSave(GetTuneF(f));
  radio->fixedBoundsMode = false;
  RADIO_SaveCurrentVFO();
}

static char message[16] = {'\0'};
static void sendDtmf() {
  RADIO_ToggleTX(true);
  if (gTxState == TX_ON) {
    BK4819_EnterDTMF_TX(true);
    BK4819_PlayDTMFString(message, true, 100, 100, 100, 100);
    RADIO_ToggleTX(false);
  }
}

void VFO1_init(void) {
  gDW.activityOnVFO = -1;
  if (!gVfo1ProMode) {
    gVfo1ProMode = gSettings.iAmPro;
  }
  RADIO_LoadCurrentVFO();
}

void VFO1_update(void) {

  if (gIsListening && Now() - gLastRender >= 500 &&
      (RADIO_GetRadio() != RADIO_SI4732 || gShowAllRSSI)) {
    gRedrawScreen = true;
  }
}

bool VFOPRO_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (key == KEY_PTT) {
    RADIO_ToggleTX(bKeyHeld);
    return true;
  }
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    case KEY_4: // freq catch
      if (RADIO_GetRadio() != RADIO_BK4819) {
        gShowAllRSSI = !gShowAllRSSI;
      }
      return true;
    case KEY_5:
      registerActive = !registerActive;
      return true;
    default:
      break;
    }
  }

  bool isSsb = RADIO_IsSSB();

  if (!bKeyHeld || bKeyPressed) {
    switch (key) {
    case KEY_1:
      RADIO_UpdateStep(true);
      return true;
    case KEY_7:
      RADIO_UpdateStep(false);
      return true;
    case KEY_3:
      RADIO_UpdateSquelchLevel(true);
      return true;
    case KEY_9:
      RADIO_UpdateSquelchLevel(false);
      return true;
    case KEY_4:
      return true;
    case KEY_0:
      RADIO_ToggleModulation();
      return true;
    case KEY_6:
      RADIO_ToggleListeningBW();
      return true;
    case KEY_F:
      gChEd = *radio;
      APPS_run(APP_CH_CFG);
      return true;
    case KEY_SIDE1:
      if (!gVfo1ProMode) {
        gMonitorMode = !gMonitorMode;
        return true;
      }
      if (RADIO_GetRadio() == RADIO_SI4732 && isSsb) {
        RADIO_TuneToSave(radio->rxF + 1);
        return true;
      }
      break;
    case KEY_SIDE2:
      if (RADIO_GetRadio() == RADIO_SI4732 && isSsb) {
        RADIO_TuneToSave(radio->rxF - 1);
        return true;
      }
      break;
    case KEY_EXIT:
      if (registerActive) {
        registerActive = false;
        return true;
      }
      break;
    default:
      break;
    }
  }

  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_2:
      if (registerActive) {
        UpdateRegMenuValue(registerSpecs[menuIndex], true);
      } else {
        IncDec8(&menuIndex, 0, ARRAY_SIZE(registerSpecs) - 1, -1);
      }
      return true;
    case KEY_8:
      if (registerActive) {
        UpdateRegMenuValue(registerSpecs[menuIndex], false);
      } else {
        IncDec8(&menuIndex, 0, ARRAY_SIZE(registerSpecs) - 1, 1);
      }
      return true;
    case KEY_5:
      gFInputCallback = tuneTo;
      APPS_run(APP_FINPUT);
      return true;
    default:
      break;
    }
  }

  return false;
}

bool VFO1_keyEx(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld,
                bool isProMode) {
  if (!SVC_Running(SVC_SCAN) && !bKeyPressed && !bKeyHeld && RADIO_IsChMode()) {
    if (!gIsNumNavInput && key <= KEY_9) {
      NUMNAV_Init(radio->channel + 1, 1, CHANNELS_GetCountMax());
      gNumNavCallback = setChannel;
    }
    if (gIsNumNavInput) {
      NUMNAV_Input(key);
      return true;
    }
  }

  if (isProMode && VFOPRO_key(key, bKeyPressed, bKeyHeld)) {
    return true;
  }

  if (key == KEY_PTT && !gIsNumNavInput) {
    RADIO_ToggleTX(bKeyHeld);
    return true;
  }

  // pressed or hold continue
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    bool isSsb = RADIO_IsSSB();
    switch (key) {
    case KEY_UP:
      SCAN_ToggleDirection(true);
      return true;
    case KEY_DOWN:
      SCAN_ToggleDirection(false);
      return true;
    case KEY_SIDE1:
      if (RADIO_GetRadio() == RADIO_SI4732 && isSsb) {
        RADIO_TuneToSave(radio->rxF + 5);
        return true;
      }
      break;
    case KEY_SIDE2:
      if (RADIO_GetRadio() == RADIO_SI4732 && isSsb) {
        RADIO_TuneToSave(radio->rxF - 5);
        return true;
      }
      break;
    default:
      break;
    }
  }

  bool longHeld = bKeyHeld && bKeyPressed && !gRepeatHeld;
  bool simpleKeypress = !bKeyPressed && !bKeyHeld;

  if (SVC_Running(SVC_SCAN) && (longHeld || simpleKeypress) &&
      (key > KEY_0 && key < KEY_9)) {
    gSettings.currentScanlist = CHANNELS_ScanlistByKey(
        gSettings.currentScanlist, key, longHeld && !simpleKeypress);
    SETTINGS_DelayedSave();
    VFO1_init();
    SCAN_Start();

    return true;
  }

  // long held
  if (longHeld) {
    OffsetDirection offsetDirection = radio->offsetDir;
    switch (key) {
    case KEY_EXIT:
      SCAN_StartAB();
      return true;
    case KEY_1:
      gChListFilter = TYPE_FILTER_BAND;
      APPS_run(APP_CH_LIST);
      return true;
    case KEY_2:
      gSettings.iAmPro = !gSettings.iAmPro;
      gVfo1ProMode = gSettings.iAmPro;
      SETTINGS_Save();
      return true;
    case KEY_3:
      RADIO_ToggleVfoMR();
      VFO1_init();
      return true;
    case KEY_4: // freq catch
      if (RADIO_GetRadio() == RADIO_BK4819) {
        SVC_Toggle(SVC_FC, !SVC_Running(SVC_FC), 100);
      } else {
        gShowAllRSSI = !gShowAllRSSI;
      }
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
      radio->offsetDir = offsetDirection;
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
      SCAN_Start();
      return true;
    case KEY_SIDE1:
      APPS_run(APP_ANALYZER);
      return true;
    case KEY_SIDE2:
      // APPS_run(APP_SPECTRUM);
      return true;
    default:
      break;
    }
  }

  // Simple keypress
  if (simpleKeypress) {
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
      gChEd = *radio;
      APPS_run(APP_CH_CFG);
      return true;
    case KEY_STAR:
      APPS_run(APP_LOOT_LIST);
      return true;
    case KEY_SIDE1:
      if (SVC_Running(SVC_SCAN)) {
        LOOT_BlacklistLast();
        RADIO_NextFreqNoClicks(true);
        return true;
      }
      gMonitorMode = !gMonitorMode;
      return true;
    case KEY_SIDE2:
      if (SVC_Running(SVC_SCAN)) {
        LOOT_WhitelistLast();
        RADIO_NextFreqNoClicks(true);
        return true;
      }
      break;
    case KEY_EXIT:
      if (SVC_Running(SVC_SCAN)) {
        SCAN_Stop();
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

bool VFO1_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  return VFO1_keyEx(key, bKeyPressed, bKeyHeld, gVfo1ProMode);
}

static void DrawRegs(void) {
  RegisterSpec rs = registerSpecs[menuIndex];

  if (rs.num == BK4819_REG_13) {
    if (radio->gainIndex == AUTO_GAIN_INDEX) {
      sprintf(String, "auto");
    } else {
      sprintf(String, "%+ddB", -gainTable[radio->gainIndex].gainDb + 33);
    }
  } else if (rs.num == 0x73) {
    uint8_t afc = BK4819_GetAFC();
    if (afc) {
      sprintf(String, "%u", afc);
    } else {
      sprintf(String, "off");
    }
  } else {
    sprintf(String, "%u", BK4819_GetRegValue(rs));
  }

  PrintMedium(2, LCD_HEIGHT - 4, "%u. %s: %s", menuIndex, rs.name, String);
  if (registerActive) {
    FillRect(0, LCD_HEIGHT - 4 - 7, LCD_WIDTH, 9, C_INVERT);
  }
}

void VFO1_render(void) {
  STATUSLINE_renderCurrentBand();

  const uint8_t BASE = 38;

  VFO *vfo = &gVFO[gSettings.activeVFO];
  uint32_t f = gTxState == TX_ON ? RADIO_GetTXF() : GetScreenF(vfo->rxF);

  uint16_t fp1 = f / MHZ;
  uint16_t fp2 = f / 100 % 1000;
  uint8_t fp3 = f % 100;
  const char *mod = modulationTypeOptions[vfo->modulation];
  if (gIsListening || gVfo1ProMode) {
    UI_RSSIBar(BASE + 2);
  }

  if (RADIO_IsChMode()) {
    PrintMediumEx(LCD_XCENTER, BASE - 16, POS_C, C_FILL, radio->name);
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

  if (gVfo1ProMode) {
    SQL sq = GetSql(radio->squelch.value);

    PrintSmall(0, 12, "R %u", RADIO_GetRSSI());
    PrintSmall(30, 12, "N %u", BK4819_GetNoise());
    PrintSmall(60, 12, "G %u", BK4819_GetGlitch());
    PrintSmall(90, 12, "SNR %u", RADIO_GetSNR());

    PrintSmallEx(LCD_WIDTH - 1, 12, POS_R, C_FILL, "SQ %u",
                 radio->squelch.value);

    PrintSmall(0, 18, "%u/%u", sq.ro, sq.rc);
    PrintSmall(30, 18, "%u/%u", sq.no, sq.nc);
    PrintSmall(60, 18, "%u/%u", sq.go, sq.gc);

    PrintSmallEx(LCD_WIDTH - 1, 18, POS_R, true,
                 RADIO_GetBWName(radio->radio, radio->bw));

    const uint32_t step = StepFrequencyTable[radio->step];
    PrintSmallEx(0, BASE, POS_L, C_FILL, "STP %d.%02d", step / 100, step % 100);

    DrawRegs();
  } else {
    FillRect(0, 22 - 14, 28, 7, C_FILL);
    PrintSmallEx(14, 22 - 9, POS_C, C_INVERT, radio->name);
  }
}
