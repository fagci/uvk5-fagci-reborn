#include "textinput.h"
#include "../driver/st7565.h"
#include "../scheduler.h"
#include "../ui/graphics.h"
#include "apps.h"
#include <string.h>

char *gTextinputText = "";
uint8_t gTextInputSize = 15;
void (*gTextInputCallback)();

static const char *letters[9] = {
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

static const char *lettersCapital[9] = {
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

static const char *numbers[10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
static const char *symbols[9] = {
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

void TEXTINPUT_init() {
  strncpy(inputField, gTextinputText, 15);
  inputIndex = strlen(inputField);
  TaskAdd("Coursor blink", blink, 250, true, 100);
}

void TEXTINPUT_deinit() { TaskRemove(blink); }

void TEXTINPUT_update() {}

bool TEXTINPUT_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {

  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (key) {
    case KEY_UP:
      if (inputIndex < 14 && inputField[inputIndex] != '\0') {
        inputIndex++;
      }
      return true;
    case KEY_DOWN:
      if (inputIndex > 0) {
        inputIndex--;
      }
      return true;
    default:
      break;
    }
  }

  // long held
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    case KEY_EXIT:
      memset(inputField, 0, 15);
      inputIndex = 0;
      return true;
    default:
      break;
    }
  }

  // Simple keypress
  if (!bKeyPressed && !bKeyHeld) {
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
        if (strlen(inputField) < gTextInputSize) {
          insert(key - KEY_0 + '0');
        }
        return true;
      }
      if (currentRow) {
        if (key - KEY_1 < strlen(currentRow)) {
          if (strlen(inputField) < gTextInputSize) {
            insert(currentRow[key - KEY_1]);
          }
          currentRow = NULL;
        }
        return true;
      }
      if (key == KEY_1) {
        if (strlen(inputField) < gTextInputSize) {
          insert(' ');
        }
      } else {
        currentRow = currentSet[key - KEY_1];
      }
      return true;
    case KEY_0:
      if (currentSet == numbers && strlen(inputField) < gTextInputSize) {
        insert(key - KEY_0 + '0');
        return true;
      }
      if (!currentRow) {
        currentSet = currentSet == numbers ? symbols : numbers;
        return true;
      }
      return true;
    case KEY_STAR:
      if (currentSet != symbols) {
        currentSet = symbols;
        return true;
      }
      return true;
    case KEY_F:
      currentSet = currentSet == lettersCapital ? letters : lettersCapital;
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
      return true;
    case KEY_MENU:
      strncpy(gTextinputText, inputField, gTextInputSize);
      if (gTextInputCallback) {
        gTextInputCallback();
        gTextInputCallback = NULL;
      }
      APPS_exit();
      return true;
    default:
      break;
    }
  }

  return false;
}

void TEXTINPUT_render() {
  char String[8];
  const uint8_t INPUT_Y = 8 + 14;
  const uint8_t CHAR_W = 5;
  const size_t charCount = strlen(inputField);

  UI_ClearScreen();

  DrawHLine(8, INPUT_Y, LCD_WIDTH - 16, C_FILL);

  PrintSmallEx(LCD_WIDTH - 8, INPUT_Y + 5, POS_R, C_FILL, "%u/%u", charCount,
               gTextInputSize);

  for (size_t i = 0; i < charCount; ++i) {
    PrintMedium(8 + i * (CHAR_W + 1), 8 + 12, "%c", inputField[i]);
  }

  uint8_t rowStrlen = 0;
  if (currentRow) {
    rowStrlen = strlen(currentRow);
  }

  if (coursorBlink) {
    DrawVLine(8 + (inputIndex * (CHAR_W + 1)) - 1, INPUT_Y - 8, 7, C_FILL);
  }

  for (uint8_t y = 0; y < 3; y++) {
    for (uint8_t x = 0; x < 3; x++) {
      const uint8_t idx = y * 3 + x;
      const uint8_t xPos = x * 43 + 1;
      const uint8_t line = y + 3;
      const uint8_t yPos = line * 8 + 12;

      PrintMedium(xPos, yPos, "%u", idx + 1);
      FillRect(xPos - 1, yPos - 6, 7, 8, C_INVERT);

      if (currentRow) {
        if (idx < rowStrlen) {
          PrintMedium(xPos + 10, line * 8 + 12, "%c", currentRow[idx]);
        }
      } else {
        strncpy(String, currentSet[idx], 4);
        String[4] = '\0';
        PrintMedium(xPos + 10, line * 8 + 12, String);
      }
    }
  }

  PrintMedium(1, LCD_HEIGHT - 2, "*");
  PrintMedium(11, LCD_HEIGHT - 2, ".,/*");
  FillRect(0, LCD_HEIGHT - 9, 7, 9, C_INVERT);

  PrintMedium(1 + 43, LCD_HEIGHT - 2, "0");
  PrintMedium(11 + 43, LCD_HEIGHT - 2, "123");
  FillRect(0 + 43, LCD_HEIGHT - 9, 7, 9, C_INVERT);

  PrintMedium(1 + 86, LCD_HEIGHT - 2, "#");
  PrintMedium(11 + 86, LCD_HEIGHT - 2,
              currentSet != lettersCapital ? "ABC" : "abc");
  FillRect(0 + 86, LCD_HEIGHT - 9, 7, 9, C_INVERT);
}


static App meta = {
    .id = APP_TEXTINPUT,
    .name = "TEXTINPUT",
    .init = TEXTINPUT_init,
    .update = TEXTINPUT_update,
    .render = TEXTINPUT_render,
    .key = TEXTINPUT_key,
    .deinit = TEXTINPUT_deinit,
};

App *TEXTINPUT_Meta() { return &meta; }
