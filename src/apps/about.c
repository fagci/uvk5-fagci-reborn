#include "about.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"

void ABOUT_Init() { STATUSLINE_SetText("t.me/uvk5_spectrum_talk"); }

void ABOUT_Deinit() {}

void ABOUT_Update() {}

void ABOUT_Render() {
  UI_ClearScreen();
  PrintMediumEx(68 + 6, 16, POS_L, C_FILL, "OSFW");
  PrintSmallEx(96 + 2, 14, POS_L, C_FILL, "REBORN");
  PrintMediumEx(LCD_WIDTH - 1, 16 + 6, POS_R, C_FILL, "FAGCI");
  PrintSmallEx(LCD_WIDTH - 29, 16 + 6, POS_R, C_FILL, "by");
}

bool ABOUT_key(KEY_Code_t k, bool p, bool h) {
  switch (k) {
  case KEY_EXIT:
    APPS_exit();
    return true;
  default:
    return false;
  }
}
