#include "about.h"
#include "../driver/st7565.h"
#include "../misc.h"
#include "../scheduler.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"

const char *qr[] = {
    "111111101111101011111101001111111", "100000101010001010101011101000001",
    "101110100000011101101001001011101", "101110101011100001111010001011101",
    "101110100010100011001000101011101", "100000100001110111011110001000001",
    "111111101010101010101010101111111", "000000001100101111110000100000000",
    "101101110110101111100101101001011", "000001010010110000110001001101111",
    "110010100100110010000011000111001", "111000000110100111100001100101011",
    "111000110101101110110011110111011", "001010001010011000111001110100110",
    "100100100010001001110000101111100", "011101001011111100000110011001100",
    "011110101111000010101111011010100", "101011000110010101001001001011011",
    "000100110001011110100000000110110", "010110010101101010110000110010001",
    "011100110000111110001100000101110", "111111011010100111100101001000101",
    "000110101000011101100101011101111", "010001010011111100111010101011011",
    "101111101111111001110010111110000", "000000001011110110010000100011010",
    "111111101101001001011001101010000", "100000101100001000000111100011101",
    "101110100100011000100111111110101", "101110101111110011000001100101001",
    "101110101111111101000001111101100", "100000100100100001111000100110001",
    "111111101101010001110101000011100",
};

void ABOUT_Init() { STATUSLINE_SetText("t.me/uvk5_spectrum_talk"); }

void ABOUT_Deinit() {
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, false);
  BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);
}

static uint32_t lastUpdate = 0;
static bool red = false;

void ABOUT_Update() {
  if (elapsedMilliseconds - lastUpdate >= 500) {
    red = (elapsedMilliseconds / 500) % 3;
    BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN,
                         (elapsedMilliseconds / 500) % 2);
    BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, red);
    lastUpdate = elapsedMilliseconds;
    gRedrawScreen = true;
  }
}

void ABOUT_Render() {
  UI_ClearScreen();
  for (uint8_t y = 0; y < ARRAY_SIZE(qr); y++) {
    for (uint8_t x = 0; x < strlen(qr[0]); x++) {
      if (qr[y][x] == '1') {
        DrawRect(x * 2, y * 1 + 9, 2, 1, C_FILL);
      }
    }
  }
  PrintMediumEx(68 + 6, 16, POS_L, C_FILL, "OSFW");
  PrintSmallEx(96 + 2, 14, POS_L, C_FILL, "REBORN");
  PrintMediumEx(LCD_WIDTH - 1, 16 + 6, POS_R, C_FILL, "FAGCI");
  PrintSmallEx(LCD_WIDTH - 29, 16 + 6, POS_R, C_FILL, "by");

  PrintMediumBoldEx(32, 50, POS_C, C_FILL, "Happy");
  PrintMediumBoldEx(32, 60, POS_C, C_FILL, "2024!");

  if (red) {
    PutPixel(LCD_WIDTH - 32, 32 - 4, C_FILL);
    PutPixel(LCD_WIDTH - 32 - 3, 32 - 1, C_FILL);
    PutPixel(LCD_WIDTH - 32 + 3, 32 - 1, C_FILL);
    PutPixel(LCD_WIDTH - 32, 32 + 2, C_FILL);
  }

  PrintSmallEx(LCD_WIDTH - 32, 32 + 2, POS_C, C_FILL, "*");
  PrintSmallEx(LCD_WIDTH - 32, 32 + 6, POS_C, C_FILL, "c");
  PrintSmallEx(LCD_WIDTH - 32, 32 + 12, POS_C, C_FILL, "ris");
  PrintSmallEx(LCD_WIDTH - 32, 32 + 18, POS_C, C_FILL, "tmas");
  PrintSmallEx(LCD_WIDTH - 32, 32 + 24, POS_C, C_FILL, "tree is");
  PrintSmallEx(LCD_WIDTH - 32, 32 + 30, POS_C, C_FILL, "here");
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

static App meta = {
    .id = APP_ABOUT,
    .name = "ABOUT",
    .runnable = true,
    .init = ABOUT_Init,
    .update = ABOUT_Update,
    .render = ABOUT_Render,
    .key = ABOUT_key,
    .deinit = ABOUT_Deinit,
};

App *ABOUT_Meta() { return &meta; }
