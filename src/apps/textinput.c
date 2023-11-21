#include "textinput.h"
#include "../driver/st7565.h"
#include "../scheduler.h"
#include "../ui/helper.h"
#include "apps.h"
#include <string.h>

char *gTextinputText;

static char *letters[9] = {
    "",
    "abc",  // 2
    "def",  // 3
    "ghi",  // 4
    "jkl",  // 5
    "mno",  // 6
    "pqrs", // 7
    "tuv",  // 8
    "wxyz"  // 9
};

static char *lettersCapital[9] = {
    "",
    "ABC",  // 2
    "DEF",  // 3
    "GHI",  // 4
    "JKL",  // 5
    "MNO",  // 6
    "PQRS", // 7
    "TUV",  // 8
    "WXYZ"  // 9
};

static char *numbers[10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
static char *symbols[9] = {
    "",
    ".,!?:;",   // 2
    "()[]<>{}", // 3
    "#@&$%",    // 4
    "+-*/=^~`", // 5
    "\"'",      // 6
    "",         // 7
    "",         // 8
    ""          // 9
};

static char **currentSet = lettersCapital;
static const char *currentRow;
static char inputField[16] = {0};
static uint8_t inputIndex = 0;
static bool coursorBlink = true;

static const uint8_t MAX_LEN = 15;

static void blink() {
  coursorBlink = !coursorBlink;
  gRedrawScreen = true;
}

static void insert(char c) {
  if (inputField[inputIndex] != '\0') {
    memmove(inputField + inputIndex + 1, inputField + inputIndex,
            16 - inputIndex);
  }
  inputField[inputIndex++] = c;
}
static void backspace() {
  if (inputField[inputIndex] != '\0') {
    inputIndex--;
    memmove(inputField + inputIndex, inputField + inputIndex + 1,
            16 - inputIndex);

  } else {
    inputField[--inputIndex] = '\0';
  }
}

void TEXTINPUT_init() { TaskAdd("Coursor blink", blink, 250, true); }
void TEXTINPUT_deinit() { TaskRemove(blink); }
void TEXTINPUT_update() {}
bool TEXTINPUT_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed || bKeyHeld) {
    return false;
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
    if (currentSet == numbers) {
      if (strlen(inputField) < MAX_LEN) {
        insert(key - KEY_0 + '0');
      }
      gRedrawScreen = true;
      return true;
    }
    if (currentRow) {
      if (key - KEY_1 < strlen(currentRow)) {
        if (strlen(inputField) < MAX_LEN) {
          insert(currentRow[key - KEY_1]);
        }
        currentRow = NULL;
      }
      gRedrawScreen = true;
      return true;
    }
    if (key == KEY_1) {
      if (strlen(inputField) < MAX_LEN) {
        insert(' ');
      }
    } else {
      currentRow = currentSet[key - KEY_1];
    }
    gRedrawScreen = true;
    return true;
  case KEY_0:
    if (currentSet == numbers) {
      insert(key - KEY_0 + '0');
      gRedrawScreen = true;
      return true;
    }
    if (!currentRow) {
      currentSet = currentSet == numbers ? symbols : numbers;
      gRedrawScreen = true;
      return true;
    }
    return true;
  case KEY_STAR:
    if (currentSet != symbols) {
      currentSet = symbols;
      gRedrawScreen = true;
      return true;
    }
    return true;
  case KEY_F:
    currentSet = currentSet == lettersCapital ? letters : lettersCapital;
    gRedrawScreen = true;
    return true;
  case KEY_UP:
    if (inputIndex < 14 && inputField[inputIndex] != '\0') {
      inputIndex++;
      gRedrawScreen = true;
    }
    return true;
  case KEY_DOWN:
    if (inputIndex > 0) {
      inputIndex--;
      gRedrawScreen = true;
    }
    return true;
  case KEY_EXIT:
    if (currentRow) {
      currentRow = NULL;
    } else {
      if (inputIndex) {
        backspace();
      } else {
        APPS_exit();
      }
    }
    gRedrawScreen = true;
    return true;
  case KEY_MENU:
    strncpy(gTextinputText, inputField, 16);
    APPS_exit();
    return true;
  default:
    break;
  }
  return false;
}
void TEXTINPUT_render() {
  char String[8];

  UI_ClearScreen();

  for (uint8_t i = 8; i < LCD_WIDTH - 8; ++i) {
    gFrameBuffer[2][i] = 0b00000001;
  }

  UI_PrintStringSmall(inputField, 8, 8, 1);

  const uint8_t CHAR_W = 6;
  uint8_t rowStrlen = 0;
  if (currentRow) {
    rowStrlen = strlen(currentRow);
  }

  if (coursorBlink) {
    gFrameBuffer[1][8 + (inputIndex * (CHAR_W + 1)) - 1] = 127;
  }

  for (uint8_t y = 0; y < 3; y++) {
    for (uint8_t x = 0; x < 3; x++) {
      const uint8_t idx = y * 3 + x;
      const uint8_t xPos = x * 43 + 1;
      const uint8_t line = y + 3;

      sprintf(String, "%u", idx + 1);
      UI_PrintStringSmall(String, xPos, xPos, line);
      for (uint8_t j = 0; j < CHAR_W + 2; ++j) {
        gFrameBuffer[line][xPos + j - 1] ^= 0xFF;
      }

      if (currentRow) {
        if (idx < rowStrlen) {
          sprintf(String, "%c", currentRow[idx]);
          UI_PrintStringSmall(String, xPos + 10, xPos + 10, line);
        }
      } else {
        strncpy(String, currentSet[idx], 4);
        String[4] = '\0';
        UI_PrintStringSmall(String, xPos + 10, xPos + 10, line);
      }
    }
  }
}
