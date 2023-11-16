#include "textinput.h"
#include "../driver/st7565.h"
#include "../ui/helper.h"

const char *letters[9] = {
    "-!@#$%&*()",
    "ABCabc",   // 2
    "DEFdef",   // 3
    "GHIghi",   // 4
    "JKLjkl",   // 5
    "MNOmno",   // 6
    "PQRSpqrs", // 7
    "TYVtuv",   // 8
    "WXYZqxyz"  // 9
};

static const char *currentRow;
static char inputField[16] = {0};
static uint8_t inputIndex = 0;

void TEXTINPUT_init() {}
void TEXTINPUT_update() {}
void TEXTINPUT_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed || bKeyHeld) {
    return;
  }
  switch (key) {
  case KEY_1:
  case KEY_2:
  case KEY_3:
  case KEY_4:
  case KEY_5:
  case KEY_6:
  case KEY_7:
  case KEY_8:
  case KEY_9:
    if (currentRow) {
      if (key - KEY_1 < strlen(currentRow)) {
        inputField[inputIndex++] = currentRow[key - KEY_1];
        currentRow = NULL;
      }
    } else {
      currentRow = letters[key - KEY_1];
    }
    gRedrawScreen = true;
    break;
  case KEY_EXIT:
    if (currentRow) {
      currentRow = NULL;
    } else {
      if (inputIndex) {
        inputField[--inputIndex] = '\0';
      }
    }
    gRedrawScreen = true;
    break;
  default:
    break;
  }
}
void TEXTINPUT_render() {
  char String[16];

  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  for (uint8_t i = 8; i < LCD_WIDTH - 8; ++i) {
    gFrameBuffer[3][i] = 0b00000001;
  }

  UI_PrintStringSmall(inputField, 8, 8, 2);

  const uint8_t CW = 8;

  if (currentRow) {
    for (uint8_t i = 0; i < strlen(currentRow); ++i) {
      const uint8_t xPos = i * (CW * 2) + 1;
      sprintf(String, "%u", i + 1);
      UI_PrintStringSmall(String, xPos, xPos, 6);
      sprintf(String, "%c", currentRow[i]);
      UI_PrintStringSmall(String, xPos, xPos, 5);
      for (uint8_t j = 0; j < CW; ++j) {
        gFrameBuffer[6][(i * CW * 2) + j] ^= 0xFF;
      }
    }
  }
}
