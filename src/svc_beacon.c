#include "svc_beacon.h"
#include "driver/bk4819.h"
#include "radio.h"

static const uint8_t M[] = {100, 100, 0, 0};

void SVC_BEACON_Init() {}
void SVC_BEACON_Update() {
  RADIO_ToggleTX(true);
  BK4819_PlaySequence(M);
  RADIO_ToggleTX(false);
}
void SVC_BEACON_Deinit() {}
