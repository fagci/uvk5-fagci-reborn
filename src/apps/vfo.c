#include "vfo.h"
#include "../scheduler.h"
#include "../ui/graphics.h"

void VFO_init() { UI_ClearScreen(); }

static bool cv = false;

void VFO_update() {
  bool cvn = (elapsedMilliseconds / 2000) % 2;
  if (cv != cvn) {
    gRedrawScreen = true;
  }
  cv = cvn;
}

bool VFO_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed) {
    return false;
  }
  return false;
}

void render1VFO() {
  const uint8_t BASE = 38;
  if (cv) {
    PrintBigDigitsEx(LCD_WIDTH - 1 - 18, BASE, POS_R, C_FILL, "123.456");
    PrintMediumEx(LCD_WIDTH - 1 - 18, BASE + 8, POS_R, C_FILL, "1234.567");
    PrintSmallEx(LCD_WIDTH - 1, BASE + 8, POS_R, C_FILL, "89");
    PrintSmallEx(LCD_WIDTH - 1, BASE - 10, POS_R, C_FILL, "WFM");
    PrintMediumEx(LCD_WIDTH - 1, BASE, POS_R, C_FILL, "78");
  } else {
    PrintBigDigitsEx(LCD_WIDTH - 1 - 18, BASE, POS_R, C_FILL, "1234.567");
    PrintMediumEx(LCD_WIDTH - 1 - 18, BASE + 8, POS_R, C_FILL, "123.456");
    PrintSmallEx(LCD_WIDTH - 1, BASE + 8, POS_R, C_FILL, "78");
    PrintSmallEx(LCD_WIDTH - 1, BASE - 10, POS_R, C_FILL, "WFM");
    PrintMediumEx(LCD_WIDTH - 1, BASE, POS_R, C_FILL, "89");
  }
}

void VFO_render() {
  UI_ClearScreen();
  render1VFO();
}
