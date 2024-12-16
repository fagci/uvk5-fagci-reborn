#include "vfopro.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/bandlist.h"
#include "../misc.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../svc_render.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "finput.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"


void VFOPRO_render(void) {
  STATUSLINE_SetText(gCurrentBand.band.name);
  UI_FSmall(gTxState == TX_ON ? RADIO_GetTXF() : GetScreenF(radio->rx.f));
  UI_RSSIBar(gLoot[gSettings.activeVFO].rssi, RADIO_GetSNR(), radio->rx.f, 23);

  if (RADIO_GetRadio() == RADIO_BK4819) {
    DrawRegs();
  }
}
