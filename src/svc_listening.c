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

static StaticTask_t taskBuffer;
static StackType_t taskStack[configMINIMAL_STACK_SIZE + 100];

static char dtmfChars[] = "0123456789ABCD*#";
static char dtmfBuffer[16] = "";
static uint8_t dtmfBufferLength = 0;
static uint32_t lastDTMF = 0;

static const uint8_t VFOS_COUNT = 2;
static uint32_t lastRssiResetTime = 0;
static uint32_t switchBackTimer = 0;

static uint32_t lightOnTimeout = UINT32_MAX;
static bool lastListenState = false;

static TaskHandle_t handle;

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

  if (!isBeken && Now() - lastMsmUpdate < 1000) {
    return msm;
  }

  if (SCAN_IsFast() && !gIsListening) {
    // Log("BK4819_ResetRSSI");
    BK4819_SetFrequency(radio->rxF);
    BK4819_WriteRegister(BK4819_REG_30, 0x0200);
    BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);
    SYSTEM_DelayMs(SCAN_GetTimeout()); // (X_X)
    msm->snr = BK4819_GetSNR();
    msm->rssi = 0; // to make spectrum work after loot
  }
  if (gIsListening) {
    msm->rssi = RADIO_GetRSSI();
    msm->noise = BK4819_GetNoise();
    msm->glitch = BK4819_GetGlitch();
    msm->snr = BK4819_GetSNR();
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
    // Log("SVC_LISTEN toggle rx=%u(was %u), snr=%u", rx, gIsListening,
    // msm->snr);
    RADIO_ToggleRX(rx);
  }
  return msm;
}

void SVC_LISTEN_Init(void) {
  handle = xTaskCreateStatic(SVC_LISTEN_Update, "LSTN", ARRAY_SIZE(taskStack),
                             NULL, 2, taskStack, &taskBuffer);
}

void SVC_LISTEN_Update(void) {
  for (;;) {
    Log("LSTN .");

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

    if (lastListenState != gIsListening) {
      // Log("Listen=%u, f=%u", gIsListening, radio->rxF);
      lastListenState = gIsListening;
      BOARD_ToggleGreen(gSettings.brightness > 1 && gIsListening);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void SVC_LISTEN_Deinit(void) {
  if (handle != NULL) {
    vTaskDelete(handle);
  }
  RADIO_ToggleRX(false);
}
