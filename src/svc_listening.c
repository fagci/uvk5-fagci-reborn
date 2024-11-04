/**
 * @author fagci
 * @author codernov https://github.com/codernov -- DW (sync)
 * */

#include "svc_listening.h"
#include "board.h"
#include "driver/bk4819.h"
#include "driver/system.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include <stddef.h>
#include <stdint.h>

Loot *(*gListenFn)(void) = NULL;

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

  if (checkActivityOnFreq(gVFO[i].rx.f)) {
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

void SVC_LISTEN_Init(void) {
  if (!gListenFn) {
    gListenFn = RADIO_UpdateMeasurements;
  }

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

  gListenFn();
  if (gSettings.scanTimeout < 10) {
    BK4819_ResetRSSI();
  }

  bool blinkMode = RADIO_GetRadio() != RADIO_BK4819 || gMonitorMode;

  if (lastListenState != gIsListening) {
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

void SVC_LISTEN_Deinit(void) {
  gListenFn = NULL;
  RADIO_ToggleRX(false);
}
