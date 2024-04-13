#include "about.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"

void ABOUT_Init() {}

void ABOUT_Deinit() {}

void ABOUT_Update() {}

void ABOUT_Render() {
  UI_ClearScreen();
  PrintMediumEx(LCD_XCENTER, LCD_YCENTER + 8, POS_C, C_FILL, "R3B0RN");
  PrintSmallEx(LCD_XCENTER, LCD_YCENTER + 14, POS_C, C_FILL, "by FAGCI");
  PrintSmallEx(LCD_XCENTER, LCD_YCENTER + 24, POS_C, C_FILL,
               "t.me/uvk5_spectrum_talk");
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
