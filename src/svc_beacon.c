#include "svc_beacon.h"
#include "driver/bk4819.h"
#include "driver/system.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"

static const uint8_t M[] = {100, 100};

void SVC_BEACON_Init() {}
void SVC_BEACON_Update() {
  uint8_t roger = gSettings.roger;
  gSettings.roger = 0;
  RADIO_ToggleTX(true);
  for (uint8_t i = 0; i < ARRAY_SIZE(M); i += 2) {
    uint16_t note = M[i] * 10;
    uint16_t t = M[i + 1] * 10;
    if (note) {
      BK4819_TransmitTone(gSettings.toneLocal, M[i] * 10);
    }
    SYSTEM_DelayMs(M[i + 1] * 10);
    BK4819_EnterTxMute();
  }
  RADIO_ToggleTX(false);
  gSettings.roger = roger;
}
void SVC_BEACON_Deinit() {}
