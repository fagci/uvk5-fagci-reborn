#include "antenna.h"
#include "../radio.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"

static uint32_t f = 0;

static void setF(uint32_t nf) {
  f = nf;
  gRedrawScreen = true;
}

void ANTENNA_init(void) {
  RADIO_LoadCurrentVFO();
  f = gCurrentVFO->fRX;
}
void ANTENNA_update(void) {}
bool ANTENNA_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_5:
      gFInputCallback = setF;
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
void ANTENNA_render(void) {
  uint32_t lambda = 30000000 / (f / 100);
  UI_ClearScreen();
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER - 8, POS_C, C_FILL, "L=%u.%02um",
                    lambda / 100, lambda % 100);
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL, "1/4=%ucm",
                    lambda / 4);
}
void ANTENNA_deinit(void) {}
