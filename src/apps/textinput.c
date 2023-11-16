#include "textinput.h"
#include "../driver/st7565.h"
#include "../ui/helper.h"

static char *letters[9] = {
    "123",
    "abc",  // 2
    "def",  // 3
    "ghi",  // 4
    "jkl",  // 5
    "mno",  // 6
    "pqrs", // 7
    "tyv",  // 8
    "wxyz"  // 9
};

static char *lettersCapital[9] = {
    "123",
    "ABC",  // 2
    "DEF",  // 3
    "GHI",  // 4
    "JKL",  // 5
    "MNO",  // 6
    "PQRS", // 7
    "TYV",  // 8
    "WXYZ"  // 9
};

static char *numbers[0] = {};
static char *symbols[9] = {
    "",
    "-_",     // 2
    "()",     // 3
    "[]",     // 4
    "<>",     // 5
    "{}",     // 6
    "*#@!?&", // 7
    "/\\|",   // 8
    "$"       // 9
};

static char **currentLetters = lettersCapital;
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
    if (currentLetters == numbers) {
      currentLetters = symbols;
    } else {
      currentLetters = numbers;
    }
    gRedrawScreen = true;
    break;
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
      currentRow = currentLetters[key - KEY_1];
    }
    gRedrawScreen = true;
    break;
  case KEY_F:
    if (currentLetters == lettersCapital) {
      currentLetters = letters;
    } else {
      currentLetters = lettersCapital;
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
    gFrameBuffer[2][i] = 0b00000001;
  }

  UI_PrintStringSmall(inputField, 8, 8, 1);

  const uint8_t CW = 8;
  uint8_t rowStrlen = 0;
  if (currentRow) {
    rowStrlen = strlen(currentRow);
  }

  for (uint8_t y = 0; y < 3; y++) {
    for (uint8_t x = 0; x < 3; x++) {
      const uint8_t idx = y * 3 + x;
      const uint8_t xPos = x * 43 + 1;
      const uint8_t line = y + 3;

      sprintf(String, "%u", idx + 1);
      UI_PrintStringSmall(String, xPos, xPos, line);
      for (uint8_t j = 0; j < CW; ++j) {
        gFrameBuffer[line][xPos + j - 1] ^= 0xFF;
      }

      if (currentRow) {
        if (idx < rowStrlen) {
          sprintf(String, "%c", currentRow[idx]);
          UI_PrintStringSmall(String, xPos + 10, xPos + 10, line);
        }
      } else {
        memcpy(String, currentLetters[idx], 4);
        UI_PrintStringSmall(String, xPos + 10, xPos + 10, line);
      }
    }
  }
}
