#include "svc_beacon.h"
#include "driver/bk4819.h"
#include "driver/system.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"

static const uint8_t M[] = {100, 10};

void SVC_BEACON_Init() {}
void SVC_BEACON_Update() {
  uint8_t roger = gSettings.roger;
  RADIO_ToggleTX(true);
    //
  RADIO_ToggleTX(false);
}
void SVC_BEACON_Deinit() {}
