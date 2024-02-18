#include "svc_listening.h"
#include "driver/bk4819-regs.h"
#include "radio.h"
#include <stddef.h>

void (*gListenFn)(void) = NULL;

void SVC_LISTEN_Init(void) {
  if (!gListenFn) {
    gListenFn = RADIO_UpdateMeasurements;
  }

  BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_CxCSS_TAIL);
}

void SVC_LISTEN_Update(void) { gListenFn(); }

void SVC_LISTEN_Deinit(void) {
  gListenFn = NULL;
  BK4819_WriteRegister(BK4819_REG_3F, 0);
}
