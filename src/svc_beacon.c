#include "svc_beacon.h"
#include "radio.h"

static const uint8_t M[] = {100, 10};

void SVC_BEACON_Init() {}
void SVC_BEACON_Update() {
  RADIO_ToggleTX(true);
    //
  RADIO_ToggleTX(false);
}
void SVC_BEACON_Deinit() {}
