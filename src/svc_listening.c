#include "svc_listening.h"
// #include "apps/messenger.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "radio.h"
#include <stddef.h>

Loot *(*gListenFn)() = NULL;

void SVC_LISTEN_Init() {
  if (!gListenFn) {
    gListenFn = RADIO_UpdateMeasurements;
  }
  // MSG_EnableRX(true);
  uint16_t InterruptMask =
      /* BK4819_REG_3F_FSK_RX_SYNC | BK4819_REG_3F_FSK_RX_FINISHED |
      BK4819_REG_3F_FSK_FIFO_ALMOST_FULL | BK4819_REG_3F_FSK_TX_FINISHED | */
      BK4819_REG_3F_CxCSS_TAIL;

  BK4819_WriteRegister(BK4819_REG_3F, InterruptMask);
  // MSG_Init();
}

void SVC_LISTEN_Update() {
  gListenFn();
  if (radio->scan.timeout < 10) {
    BK4819_ResetRSSI();
  }
}

void SVC_LISTEN_Deinit() {
  gListenFn = NULL;
  BK4819_WriteRegister(BK4819_REG_3F, 0);
}
