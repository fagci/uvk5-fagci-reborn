#include "level.h"
#include "../helper/measurements.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/spectrum.h"
#include "finput.h"

static Loot msm;

void LEVEL_init(void) {}

void LEVEL_deinit(void) {}

static uint32_t lastUpdate = 0;

void LEVEL_update(void) {
  if (Now() - lastUpdate > 50) {
    lastUpdate = Now();
    msm.rssi = RADIO_GetRSSI();
    SP_Shift(-1);
    SP_AddGraphPoint(&msm);
    gRedrawScreen = true;
  }
}

bool LEVEL_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  // keypress
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_1:
      RADIO_UpdateStep(true);
      return true;
    case KEY_7:
      RADIO_UpdateStep(false);
      return true;
    case KEY_0:
      RADIO_ToggleModulation();
      return true;
    case KEY_6:
      RADIO_ToggleListeningBW();
      return true;
    case KEY_SIDE1:
      gMonitorMode = !gMonitorMode;
      return true;
    case KEY_F:
      APPS_run(APP_VFO_CFG);
      return true;
    case KEY_5:
      gFInputCallback = RADIO_TuneTo;
      APPS_run(APP_FINPUT);
      return true;
    case KEY_EXIT:
      APPS_exit();
      return true;
    default:
      break;
    }
  }
  return false;
}

void LEVEL_render(void) {
  UI_ClearScreen();

  UI_FSmall(radio->rx.f);

  SP_RenderGraph(0, 32, 16);
  DrawHLine(0, 32, 2, C_FILL);
  DrawHLine(0, 32 + 16, 2, C_FILL);
  PrintSmallEx(LCD_XCENTER, LCD_HEIGHT - 2, POS_C, C_FILL, "%d",
               Rssi2DBm(msm.rssi));
}
