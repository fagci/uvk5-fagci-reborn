#include "finput.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../ui/graphics.h"
#include "apps.h"
#include <string.h>

void (*gFInputCallback)(uint32_t f);

static const uint8_t FREQ_INPUT_LENGTH = 10;

static KEY_Code_t freqInputArr[10];

static uint8_t freqInputDotIndex = 0;
static uint8_t freqInputIndex = 0;

static uint32_t tempFreq;

static bool dotBlink = true;

static void ResetFreqInput() { tempFreq = 0; }

static void dotBlinkFn() {
  if (!freqInputDotIndex) {
    dotBlink = !dotBlink;
    gRedrawScreen = true;
  }
}

static void input(KEY_Code_t key) {
  if (key != KEY_EXIT && freqInputIndex >= FREQ_INPUT_LENGTH) {
    return;
  }
  if (key == KEY_STAR) {
    if (freqInputIndex == 0 || freqInputDotIndex) {
      return;
    }
    freqInputDotIndex = freqInputIndex;
  }
  if (key == KEY_EXIT) {
    if (freqInputIndex && freqInputArr[freqInputIndex - 1] == KEY_STAR) {
      freqInputDotIndex = 0;
    }
    freqInputIndex--;
  } else {
    freqInputArr[freqInputIndex++] = key;
  }

  ResetFreqInput();

  uint8_t dotIndex =
      freqInputDotIndex == 0 ? freqInputIndex : freqInputDotIndex;

  uint32_t base = 100000; // 1MHz in BK units
  for (int i = dotIndex - 1; i >= 0; --i) {
    tempFreq += freqInputArr[i] * base;
    base *= 10;
  }

  base = 10000; // 0.1MHz in BK units
  if (dotIndex < freqInputIndex) {
    for (uint8_t i = dotIndex + 1; i < freqInputIndex; ++i) {
      tempFreq += freqInputArr[i] * base;
      base /= 10;
    }
  }
}

void FINPUT_init() {
  UI_ClearStatus();
  freqInputIndex = 0;
  freqInputDotIndex = 0;
  ResetFreqInput();
  TaskAdd("Dot blink", dotBlinkFn, 250, true);
}

void FINPUT_deinit() { TaskRemove(dotBlinkFn); }

bool FINPUT_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (bKeyHeld) {
    return false;
  }
  switch (key) {
  case KEY_0:
  case KEY_1:
  case KEY_2:
  case KEY_3:
  case KEY_4:
  case KEY_5:
  case KEY_6:
  case KEY_7:
  case KEY_8:
  case KEY_9:
  case KEY_STAR:
    input(key);
    if (tempFreq > 13000000) {
      input(KEY_STAR);
    }
    gRedrawScreen = true;
    return true;
  case KEY_EXIT:
    if (freqInputIndex == 0) {
      gFInputCallback = NULL;
      APPS_exit();
      return true;
    }
    input(key);
    gRedrawScreen = true;
    return true;
  case KEY_MENU:
  case KEY_F:
  case KEY_PTT:
    tempFreq = GetTuneF(tempFreq);
    if (tempFreq <= F_MAX && gFInputCallback) {
      APPS_exit();
      gFInputCallback(tempFreq);
      gFInputCallback = NULL;
    }
    return true;
  default:
    break;
  }
  return false;
}

void FINPUT_render() {
  UI_ClearScreen();

  uint8_t dotIndex = freqInputDotIndex ? freqInputDotIndex : freqInputIndex;

  const uint8_t charWidth = 10;
  const uint8_t BASE_X = 16;
  const uint8_t BASE_Y = 24;
  uint8_t i = 0;

  // MHz
  while (i < 4) {
    if (i >= 4 - dotIndex) {
      const unsigned int Digit = freqInputArr[i - (4 - dotIndex)];
      if (Digit < 10) {
        PrintBigDigits(BASE_X + i * charWidth, BASE_Y, "%c", '0' + Digit);
      }
    }
    i++;
  }

  // decimal point
  if (freqInputDotIndex || (!freqInputDotIndex && dotBlink)) {
    PrintBigDigits(BASE_X + i * charWidth, BASE_Y, "%c", '.');
    i++;
  }

  // kHz
  if (freqInputDotIndex) {
    i = dotIndex + 1;
    while (i < freqInputIndex && i < dotIndex + 6) {
      const uint8_t Digit = freqInputArr[i];
      PrintBigDigits(BASE_X + (4 + i - dotIndex) * charWidth - 6, BASE_Y, "%c",
                     '0' + Digit);
      i++;
    }
  }

  /* char String[] = "  ";
  if (freqInputDotIndex) {
    uint8_t hz = freqInputIndex - freqInputDotIndex;
    if (hz > 3) {
      String[0] = hz > 4 ? '0' + freqInputArr[freqInputDotIndex + 4] : ' ';
      String[1] = hz > 5 ? '0' + freqInputArr[freqInputDotIndex + 5] : ' ';
      PrintMediumEx(LCD_WIDTH - 1 - 18, BASE_Y, POS_L, C_FILL, String);
    }
  } */
}
