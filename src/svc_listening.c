#include "svc_listening.h"
#include "dcs.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "driver/uart.h"
#include "radio.h"
#include <stddef.h>

void (*gListenFn)(void) = NULL;

static void setupToneDetection() {
  uint16_t InterruptMask = BK4819_REG_3F_CxCSS_TAIL;
  switch (radio->rx.codeType) {
  case CODE_TYPE_DIGITAL:
  case CODE_TYPE_REVERSE_DIGITAL:
    Log("RX dc en");
    BK4819_SetCDCSSCodeWord(
        DCS_GetGolayCodeWord(radio->rx.codeType, radio->rx.code));
    InterruptMask |= BK4819_REG_3F_CDCSS_FOUND | BK4819_REG_3F_CDCSS_LOST;
    break;
  case CODE_TYPE_CONTINUOUS_TONE:
    Log("RX ct en");
    BK4819_SetCTCSSFrequency(CTCSS_Options[radio->rx.code]);
    InterruptMask |= BK4819_REG_3F_CTCSS_FOUND | BK4819_REG_3F_CTCSS_LOST;
    break;
  default:
    BK4819_SetCTCSSFrequency(670); // ?
    break;
  }
  BK4819_WriteRegister(BK4819_REG_3F, InterruptMask);
}

void SVC_LISTEN_Init(void) {
  if (!gListenFn) {
    gListenFn = RADIO_UpdateMeasurements;
  }
  setupToneDetection();
}

void SVC_LISTEN_Update(void) {
  gListenFn();
  if (gSettings.scanTimeout < 10) {
    BK4819_ResetRSSI();
  }
}

void SVC_LISTEN_Deinit(void) {
  gListenFn = NULL;
  BK4819_WriteRegister(BK4819_REG_3F, 0);
  RADIO_ToggleRX(false);
}
