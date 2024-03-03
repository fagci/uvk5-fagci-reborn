#include "vfo1.h"
#include "../helper/lootlist.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../svc.h"
#include "../svc_scan.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"
#include "vfo2.h"

static uint32_t lastUpdate = 0;

void VFO1_init(void) {
  RADIO_LoadCurrentVFO();
  gRedrawScreen = true;
}

void VFO1_deinit(void) {}

void VFO1_update(void) {
  if (elapsedMilliseconds - lastUpdate >= 500) {
    gRedrawScreen = true;
    lastUpdate = elapsedMilliseconds;
  }
}

bool VFO1_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  return VFO2_key(key, bKeyPressed, bKeyHeld);
}

void VFO1_render(void) {
  UI_ClearScreen();
  const uint8_t BASE = 38;

  VFO *vfo = &gVFO[gSettings.activeVFO];
  Preset *p = gVFOPresets[gSettings.activeVFO];
  uint32_t f = gTxState == TX_ON ? RADIO_GetTXF() : GetScreenF(vfo->rx.f);

  uint16_t fp1 = f / 100000;
  uint16_t fp2 = f / 100 % 1000;
  uint8_t fp3 = f % 100;
  const char *mod = modulationTypeOptions[p->band.modulation];
  if (gIsListening) {
    if (!isBK1080) {
      UI_RSSIBar(gLoot[gSettings.activeVFO].rssi, vfo->rx.f, BASE + 2);
    }
  }

  if (gTxState && gTxState != TX_ON) {
    PrintMediumBoldEx(LCD_XCENTER, BASE, POS_C, C_FILL, "%s",
                      TX_STATE_NAMES[gTxState]);
  } else {
    PrintBiggestDigitsEx(LCD_WIDTH - 22, BASE, POS_R, C_FILL, "%4u.%03u", fp1,
                         fp2);
    PrintBigDigitsEx(LCD_WIDTH - 1, BASE, POS_R, C_FILL, "%02u", fp3);
    PrintMediumEx(LCD_WIDTH - 1, BASE - 12, POS_R, C_FILL, mod);
  }
}

static VFO vfo;

App meta = {
    .id = APP_VFO1,
    .name = "VFO 1",
    .init = VFO1_init,
    .update = VFO1_update,
    .render = VFO1_render,
    .key = VFO1_key,
    .deinit = VFO1_deinit,
    .vfo = &vfo,
};

App *VFO1_Meta(void) { return &meta; }
