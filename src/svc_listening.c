/**
 * @author fagci
 * @author codernov https://github.com/codernov -- DW (sync)
 * */

#include "svc_listening.h"
#include "board.h"
#include "dcs.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "driver/si473x.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "driver/uart.h"
#include "helper/battery.h"
#include "helper/lootlist.h"
#include "helper/scan.h"
#include "misc.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include "svc.h"
#include "ui/spectrum.h"
#include "ui/statusline.h"
#include <stdint.h>
#include <string.h>

static char dtmfChars[] = "0123456789ABCD*#";
static char dtmfBuffer[16] = "";
static uint8_t dtmfBufferLength = 0;
static uint32_t lastDTMF = 0;

static const uint8_t VFOS_COUNT = 2;
static const uint8_t DW_CHECK_DELAY = 20;
static const uint16_t DW_RSSI_RESET_CYCLE = 1000;
static uint32_t lastRssiResetTime = 0;
static uint32_t switchBackTimer = 0;

static uint32_t lightOnTimeout = UINT32_MAX;
static uint32_t blinkTimeout = UINT32_MAX;
static bool lastListenState = false;

static const uint32_t BLINK_DURATION = 50;
static const uint32_t BLINK_INTERVAL = 5000;

static bool checkActivityOnFreq(uint32_t freq) {
  // TODO: rewrite to use base RADIO logic
  uint32_t oldFreq = BK4819_GetFrequency();
  bool activity;

  BK4819_TuneTo(freq, false);
  SYSTEM_DelayMs(DW_CHECK_DELAY);
  activity = BK4819_IsSquelchOpen();
  BK4819_TuneTo(oldFreq, false);

  return activity;
}

static void sync(void) {
  static int8_t i = VFOS_COUNT - 1;

  if (checkActivityOnFreq(gVFO[i].rxF)) {
    gDW.activityOnVFO = i;
    gDW.lastActiveVFO = i;
    if (gSettings.dw == DW_SWITCH) {
      gSettings.activeVFO = i;
      RADIO_SaveCurrentVFO();
    }
    gDW.isSync = true;
    gDW.doSync = false;
    gDW.doSwitch = true;
    gDW.doSwitchBack = false;
  }

  i--;
  if (i < 0) {
    i = VFOS_COUNT - 1;
  }
}

static uint32_t lastMsmUpdate = 0;
static uint32_t lastTailTone = 0;
static bool toneFound = false;
static uint8_t lastSNR = 0;

static Measurement *updateMeasurements(void) {
  Measurement *msm = &gLoot[gSettings.activeVFO];
  if (RADIO_GetRadio() == RADIO_SI4732 && SVC_Running(SVC_SCAN)) {
    bool valid = false;
    uint32_t f = SI47XX_getFrequency(&valid);
    radio->rxF = f;
    gRedrawScreen = true;
    if (valid) {
      SVC_Toggle(SVC_SCAN, false, 0);
    }
  }

  const bool isBeken = RADIO_GetRadio() == RADIO_BK4819;
  const bool isFastScanListen = isBeken && gIsListening && SCAN_IsFast();

  if (isFastScanListen && Now() - lastMsmUpdate < 1000) {
    return msm;
  }

  if (!isBeken && Now() - lastMsmUpdate < 1000) {
    return msm;
  }

  if (SCAN_IsFast()) {
    BK4819_SetFrequency(radio->rxF);
    BK4819_WriteRegister(BK4819_REG_30, 0x0200);
    BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);
    SYSTEM_DelayMs(SCAN_GetTimeout()); // (X_X)
    msm->snr = BK4819_GetSNR();
    msm->rssi = 0; // to make spectrum work after loot
  }
  if (gIsListening) {
    msm->rssi = RADIO_GetRSSI();
    // msm->noise = BK4819_GetNoise();
    // msm->glitch = BK4819_GetGlitch();
  }
  lastMsmUpdate = Now();
  msm->open = RADIO_IsSquelchOpen(msm);
  // Log("U MSM, o=%u, r=%u", msm->open, msm->snr);

  if (radio->code.rx.type == CODE_TYPE_OFF) {
    toneFound = true;
  }

  if (!(SVC_Running(SVC_SCAN) && SCAN_IsFast()) &&
      RADIO_GetRadio() == RADIO_BK4819) {
    while (BK4819_ReadRegister(BK4819_REG_0C) & 1u) {
      BK4819_WriteRegister(BK4819_REG_02, 0);

      uint16_t intBits = BK4819_ReadRegister(BK4819_REG_02);

      if ((intBits & BK4819_REG_02_CxCSS_TAIL) ||
          (intBits & BK4819_REG_02_CTCSS_FOUND) ||
          (intBits & BK4819_REG_02_CDCSS_FOUND)) {
        // Log("Tail tone or ctcss/dcs found");
        msm->open = false;
        toneFound = false;
        lastTailTone = Now();
      }
      if ((intBits & BK4819_REG_02_CTCSS_LOST) ||
          (intBits & BK4819_REG_02_CDCSS_LOST)) {
        // Log("ctcss/dcs lost");
        msm->open = true;
        toneFound = true;
      }

      if (intBits & BK4819_REG_02_DTMF_5TONE_FOUND) {
        uint8_t code = BK4819_GetDTMF_5TONE_Code();
        // Log("DTMF: %u", code);
        lastDTMF = Now();
        lastSNR = RADIO_GetSNR();
        dtmfBuffer[dtmfBufferLength++] = dtmfChars[code];
      }
    }
    if (Now() - lastDTMF > 1000 && dtmfBufferLength) {
      // make an actions with buffer
      // STATUSLINE_SetTickerText("DTMF: %s", dtmfBuffer);
      if (dtmfBuffer[0] == 'A') {
        // Log("A CMD");
        switch (dtmfBuffer[1]) {
        case '1':
          SVC_Toggle(SVC_BEACON, !SVC_Running(SVC_BEACON), 15000);
          RADIO_SendDTMF("00");
          break;
        case '2':
          RADIO_SendDTMF("12345");
          break;
        case '3':
          RADIO_SendDTMF("%u", lastSNR);
          break;
        case '4':
          RADIO_SendDTMF("%u", gBatteryPercent);
          break;
        default:
          RADIO_SendDTMF("99");
          break;
        }
      }
      dtmfBufferLength = 0;
      memset(dtmfBuffer, 0, ARRAY_SIZE(dtmfBuffer));
    }
    // else sql reopens
    if (!toneFound || (Now() - lastTailTone) < 250) {
      msm->open = false;
    }
  }

  if (RADIO_GetRadio() == RADIO_BK4819) {
    LOOT_Update(msm);
  }

  bool rx = msm->open;
  if (gTxState != TX_ON) {
    if (gMonitorMode) {
      rx = true;
    } else if (gSettings.noListen && gCurrentApp == APP_ANALYZER) {
      rx = false;
    } else if (gSettings.skipGarbageFrequencies &&
               (radio->rxF % 1300000 == 0) &&
               RADIO_GetRadio() == RADIO_BK4819) {
      rx = false;
    }
    RADIO_ToggleRX(rx);
  }
  return msm;
}

void SVC_LISTEN_Init(void) {
  gDW.lastActiveVFO = -1;
  gDW.activityOnVFO = 0;
  gDW.isSync = false;
  gDW.doSync = gSettings.dw != DW_OFF;
  gDW.doSwitch = false;
  gDW.doSwitchBack = false;
}

void SVC_LISTEN_Update(void) {
  if (gSettings.dw != DW_OFF) {
    if (gDW.doSwitch) {
      gDW.doSwitch = false;
      gDW.doSync = false;
      gDW.doSwitchBack = true;

      radio = &gVFO[gDW.activityOnVFO];
      RADIO_SetupByCurrentVFO();
    }

    if (gDW.doSwitchBack) {
      if (!gIsListening && Now() - switchBackTimer >= 500) {
        switchBackTimer = Now();

        gDW.doSwitch = false;
        gDW.doSync = true;
        gDW.doSwitchBack = false;
        gDW.isSync = false;

        radio = &gVFO[gSettings.activeVFO];
        RADIO_SetupByCurrentVFO();
      }
    }
    // return;
  }
  if (gTxState == TX_UNKNOWN && gDW.doSync) {
    if (gSettings.dw == DW_OFF) {
      gDW.doSync = false;
      return;
    }

    sync();

    if (Now() - lastRssiResetTime > DW_RSSI_RESET_CYCLE) {
      BK4819_ResetRSSI();
      lastRssiResetTime = Now();
    }
    return;
  }

  // if (RADIO_GetRadio() == RADIO_BK4819 || gShowAllRSSI) {
  Measurement *m = updateMeasurements();
  if (gIsListening) {
    static uint32_t lastGraphMsm;
    if (Now() - lastGraphMsm > 250) {
      SP_ShiftGraph(-1);
      SP_AddGraphPoint(m);
      lastGraphMsm = Now();
    }
  } else {
    SP_AddPoint(m);
  }
  // }

  bool blinkMode = RADIO_GetRadio() != RADIO_BK4819 || gMonitorMode;

  if (lastListenState != gIsListening) {
    // Log("Listen=%u, f=%u", gIsListening, radio->rxF);
    lastListenState = gIsListening;
    BOARD_ToggleGreen(gSettings.brightness > 1 && gIsListening);
    if (gIsListening && blinkMode) {
      blinkTimeout = Now() + BLINK_DURATION + BLINK_INTERVAL;
    }
  }

  if (!blinkMode) {
    return;
  }

  if (gIsListening && Now() > blinkTimeout) {
    BOARD_ToggleGreen(true);
    lightOnTimeout = Now() + BLINK_DURATION;
    blinkTimeout = Now() + BLINK_DURATION + BLINK_INTERVAL;
  }

  if (Now() > lightOnTimeout) {
    BOARD_ToggleGreen(false);
    lightOnTimeout = UINT32_MAX;
  }
}

void SVC_LISTEN_Deinit(void) { RADIO_ToggleRX(false); }
