#include "vfo1.h"
#include "../apps/textinput.h"
#include "../driver/bk4819.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../svc.h"
#include "../svc_render.h"
#include "../svc_scan.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "chcfg.h"
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
    v = gCurrentPreset.gainIndex;
    maxValue = ARRAY_SIZE(gainTable) - 1;
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

  if (s.num == BK4819_REG_13) {
    RADIO_SetGain(v);
  } else if (s.num == 0x73) {
    BK4819_SetAFC(v);
  } else {
    BK4819_SetRegValue(s, v);
  }
}

static void setChannel(uint16_t v) { RADIO_TuneToCH(v - 1); }
static void tuneTo(uint32_t f) { RADIO_TuneToSave(GetTuneF(f)); }

static bool SSB_Seek_ON = false;
static bool SSB_Seek_UP = true;

static int32_t scanIndex = 0;

static char message[16] = {'\0'};
static void sendDtmf() {
  RADIO_ToggleTX(true);
  if (gTxState == TX_ON) {
    BK4819_EnterDTMF_TX(true);
    BK4819_PlayDTMFString(message, true, 100, 100, 100, 100);
    RADIO_ToggleTX(false);
  }
}

static void channelScanFn(bool forward) {
  IncDecI32(&scanIndex, 0, gScanlistSize, forward ? 1 : -1);
  int32_t chNum = gScanlist[scanIndex];
  radio->channel = chNum;
  RADIO_VfoLoadCH(gSettings.activeVFO);
  RADIO_SetupByCurrentVFO();
}

void VFO1_init(void) {
  gDW.activityOnVFO = -1;
  if (!gVfo1ProMode) {
    gVfo1ProMode = gSettings.iAmPro;
  }
  RADIO_LoadCurrentVFO();
}

void VFO1_update(void) {
  if (SSB_Seek_ON) {
    if (RADIO_GetRadio() == RADIO_SI4732 && RADIO_IsSSB()) {
      if (Now() - gLastRender >= 150) {
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
    if (gVfo1ProMode) {
      RADIO_UpdateMeasurements();
    }
  }
}

static void prepareABScan() {
  uint32_t F1 = gVFO[0].rxF;
  uint32_t F2 = gVFO[1].rxF;

  if (F1 > F2) {
    SWAP(F1, F2);
  }

  defaultPreset.rxF = F1;
  defaultPreset.txF = F2;

  sprintf(defaultPreset.name, "%u-%u", F1 / MHZ, F2 / MHZ);
  gCurrentPreset = defaultPreset;
  defaultPreset.misc.lastUsedFreq = radio->rxF;
  gSettings.crossBandScan = false;
  RADIO_TuneToPure(F1, true);
}

static void initChannelScan() {
  scanIndex = 0;
  LOOT_Clear();
  CHANNELS_LoadScanlist(gSettings.currentScanlist);
  if (gScanlistSize == 0) {
    gSettings.currentScanlist = 15;
    CHANNELS_LoadScanlist(gSettings.currentScanlist);
    SETTINGS_DelayedSave();
  }
  for (uint16_t i = 0; i < gScanlistSize; ++i) {
    CH ch;
    int32_t num = gScanlist[i];
    CHANNELS_Load(num, &ch);
    Loot *loot = LOOT_AddEx(ch.rxF, true);
    loot->open = false;
    loot->lastTimeOpen = 0;
  }

  gScanFn = channelScanFn;
}

static void startScan() {
  if (radio->channel >= 0) {
    initChannelScan();
  }
  if (radio->channel >= 0 && gScanlistSize == 0) {
    SVC_Toggle(SVC_SCAN, false, 0);
    return;
  }
  SVC_Toggle(SVC_SCAN, true, gSettings.scanTimeout);
}

static void scanlistByKey(KEY_Code_t key) {
  if (key >= KEY_1 && key <= KEY_8) {
    gSettings.currentScanlist = key - 1;
  } else {
    gSettings.currentScanlist = 15;
  }
}

static void selectFirstPresetFromScanlist() {
  uint8_t sl = gSettings.currentScanlist;
  uint8_t scanlistMask = 1 << sl;
  for (uint8_t i = 0; i < PRESETS_COUNT_MAX; ++i) {
    if (sl == 15 ||
        (PRESETS_Item(i).memoryBanks & scanlistMask) == scanlistMask) {
      PRESET_Select(i);
      RADIO_TuneTo(gCurrentPreset.rxF);
      return;
    }
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
      APPS_exit();
      return true;
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
      gFInputCallback = RADIO_TuneToSave;
      APPS_run(APP_FINPUT);
      return true;
    default:
      break;
    }
  }

  return false;
}

bool VFO1_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!SVC_Running(SVC_SCAN) && !bKeyPressed && !bKeyHeld &&
      radio->channel >= 0) {
    if (!gIsNumNavInput && key <= KEY_9) {
      NUMNAV_Init(radio->channel + 1, 1, CHANNELS_GetCountMax());
      gNumNavCallback = setChannel;
    }
    if (gIsNumNavInput) {
      NUMNAV_Input(key);
      return true;
    }
  }

  if (gVfo1ProMode && VFOPRO_key(key, bKeyPressed, bKeyHeld)) {
    return true;
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
      if (SSB_Seek_ON) {
        SSB_Seek_UP = true;
        return true;
      }
      if (SVC_Running(SVC_SCAN)) {
        gScanForward = true;
      }
      RADIO_NextFreqNoClicks(true);
      return true;
    case KEY_DOWN:
      if (SSB_Seek_ON) {
        SSB_Seek_UP = false;
        return true;
      }
      if (SVC_Running(SVC_SCAN)) {
        gScanForward = false;
      }
      RADIO_NextFreqNoClicks(false);
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

  // long held
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    OffsetDirection offsetDirection = gCurrentPreset.offsetDir;
    switch (key) {
    case KEY_EXIT:
      prepareABScan();
      startScan();
      return true;
    case KEY_1:
      APPS_run(APP_CH_LIST);
      return true;
    case KEY_2:
      gSettings.iAmPro = !gSettings.iAmPro;
      gVfo1ProMode = gSettings.iAmPro;
      SETTINGS_Save();
      return true;
    case KEY_3:
      RADIO_ToggleVfoMR();
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
      gCurrentPreset.offsetDir = offsetDirection;
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
        SSB_Seek_ON = true;
        // todo: scan by snr
      } else {
        if (gSettings.crossBandScan && radio->channel <= -1) {
          selectFirstPresetFromScanlist();
        }
        startScan();
      }
      return true;
    case KEY_SIDE1:
      APPS_run(APP_ANALYZER);
      return true;
    case KEY_SIDE2:
      APPS_run(APP_SPECTRUM);
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
        if (radio->channel <= -1) {
          if (gSettings.crossBandScan) {
            scanlistByKey(key);
            selectFirstPresetFromScanlist();
          } else {
            RADIO_SelectPresetSave(key + 6);
          }
        } else {
          scanlistByKey(key);
          initChannelScan();
          SETTINGS_DelayedSave();
        }
      } else {
        gFInputCallback = tuneTo;
        APPS_run(APP_FINPUT);
        APPS_key(key, bKeyPressed, bKeyHeld);
      }
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
      if (SSB_Seek_ON) {
        SSB_Seek_ON = false;
        return true;
      }
      if (SVC_Running(SVC_SCAN)) {
        SVC_Toggle(SVC_SCAN, false, 0);
        return true;
      }
      if (!APPS_exit()) {
        SVC_Toggle(SVC_SCAN, false, 0);
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
  const uint8_t BASE = 38;

  VFO *vfo = &gVFO[gSettings.activeVFO];
  uint32_t f = gTxState == TX_ON ? RADIO_GetTXF() : GetScreenF(vfo->rxF);

  uint16_t fp1 = f / MHZ;
  uint16_t fp2 = f / 100 % 1000;
  uint8_t fp3 = f % 100;
  const char *mod = modulationTypeOptions[vfo->modulation];
  if (gIsListening || gVfo1ProMode) {
    UI_RSSIBar(gLoot[gSettings.activeVFO].rssi, RADIO_GetSNR(), vfo->rxF,
               BASE + 2);
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

  if (gVfo1ProMode) {
    UI_FSmall(gTxState == TX_ON ? RADIO_GetTXF() : GetScreenF(radio->rxF));
    DrawRegs();
  }
}
