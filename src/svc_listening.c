#include "svc_listening.h"
#include "driver/bk4819.h"
#include "radio.h"
#include <stddef.h>

void (*gListenFn)(void) = NULL;

void SVC_LISTEN_Init(void) {
  if (!gListenFn) {
    gListenFn = RADIO_UpdateMeasurements;
  }
}

void SVC_LISTEN_Update(void) {
  gListenFn();
  if (gSettings.scanTimeout < 10) {
    BK4819_ResetRSSI();
  }
}

void SVC_LISTEN_Deinit(void) {
  gListenFn = NULL;
  RADIO_ToggleRX(false);
}
