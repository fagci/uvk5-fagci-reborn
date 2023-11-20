#include "finput.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../font.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../ui/helper.h"
#include "apps.h"
#include <string.h>

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
  memset(gStatusLine, 0, sizeof(gStatusLine));
  freqInputIndex = 0;
  freqInputDotIndex = 0;
  ResetFreqInput();
  TaskAdd("Dot blink", dotBlinkFn, 250, true);
}

void FINPUT_deinit() {
  TaskRemove(dotBlinkFn);
  APPS_exit();
}

bool FINPUT_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed || bKeyHeld) {
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
      FINPUT_deinit();
      return true;
    }
    input(key);
    gRedrawScreen = true;
    return true;
  case KEY_MENU:
    tempFreq = GetTuneF(tempFreq);
    if (tempFreq >= F_MIN && tempFreq <= F_MAX) {
      RADIO_TuneTo(tempFreq, true);
    }
    FINPUT_deinit();
    return true;
  default:
    break;
  }
  return false;
}

void FINPUT_render() {
  char String[16];
  UI_ClearScreen();

  const uint8_t X = 19;
  const uint8_t Y = 0;
  uint8_t dotIndex =
      freqInputDotIndex == 0 ? freqInputIndex : freqInputDotIndex;

  const unsigned int charWidth = 13;
  uint8_t *pFb0 = gFrameBuffer[Y] + X;
  uint8_t *pFb1 = pFb0 + 128;
  uint8_t i = 0;

  // MHz
  while (i < 4) {
    if (i >= 4 - dotIndex) {
      const unsigned int Digit = freqInputArr[i - (4 - dotIndex)];
      if (Digit < 10) {
        memmove(pFb0, gFontBigDigits[Digit], charWidth);
        memmove(pFb1, gFontBigDigits[Digit] + charWidth, charWidth);
      }
    }
    i++;
    pFb0 += charWidth;
    pFb1 += charWidth;
  }

  // decimal point
  if (freqInputDotIndex || (!freqInputDotIndex && dotBlink)) {
    *pFb1 = 0x60;
    pFb1++;
    *pFb1 = 0x60;
    pFb1++;
    *pFb1 = 0x60;
    pFb1++;
  } else {
    pFb1 += 3;
  }
  pFb0 += 3;

  // kHz
  if (freqInputDotIndex) {
    i = dotIndex + 1;
    while (i < freqInputIndex && i < dotIndex + 4) {
      const uint8_t Digit = freqInputArr[i];
      memmove(pFb0, gFontBigDigits[Digit], charWidth);
      memmove(pFb1, gFontBigDigits[Digit] + charWidth, charWidth);
      i++;
      pFb0 += charWidth;
      pFb1 += charWidth;
    }
  }

  if (freqInputDotIndex) {
    uint8_t hz = freqInputIndex - freqInputDotIndex;
    if (hz > 3) {
      String[0] = hz > 4 ? freqInputArr[freqInputDotIndex + 4] : 11;
      String[1] = hz > 5 ? freqInputArr[freqInputDotIndex + 5] : 11;
      UI_DisplaySmallDigits(2, String, 113, 1);
    }
  }
}
