#include "svc_listening.h"
#include "board.h"
#include "driver/bk4819.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include <stddef.h>
#include <stdint.h>

Loot *(*gListenFn)(void) = NULL;

void SVC_LISTEN_Init(void) {
  if (!gListenFn) {
    gListenFn = RADIO_UpdateMeasurements;
  }
}

static uint32_t lightOnTimeout = UINT32_MAX;
static uint32_t blinkTimeout = UINT32_MAX;
static bool lastListenState = false;

static const uint32_t BLINK_DURATION = 50;
static const uint32_t BLINK_INTERVAL = 5000;

void SVC_LISTEN_Update(void) {
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
