#include "finput.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../radio.h"
#include "../ui/helper.h"
#include "apps.h"
#include <string.h>

static KEY_Code_t freqInputArr[10];
static uint8_t freqInputDotIndex = 0;
static const uint8_t FREQ_INPUT_LENGTH = 10;
static char freqInputString[] = "----------"; // XXXX.XXXXX
static uint8_t freqInputIndex = 0;
static uint32_t tempFreq;
static uint32_t gInputFreq = 0;
static AppType_t previousApp;

static void ResetFreqInput() {
  tempFreq = 0;
  memset(freqInputString, '-', FREQ_INPUT_LENGTH);
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
    if (freqInputArr[freqInputIndex] == KEY_STAR) {
      freqInputDotIndex = 0;
    }
    freqInputIndex--;
  } else {
    freqInputArr[freqInputIndex++] = key;
  }

  ResetFreqInput();

  uint8_t dotIndex =
      freqInputDotIndex == 0 ? freqInputIndex : freqInputDotIndex;

  KEY_Code_t digitKey;
  for (uint8_t i = 0; i < 10; ++i) {
    if (i < freqInputIndex) {
      digitKey = freqInputArr[i];
      freqInputString[i] = digitKey <= KEY_9 ? '0' + digitKey : '.';
    } else {
      freqInputString[i] = '-';
    }
  }

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
  previousApp = gCurrentApp;
  freqInputIndex = 0;
  freqInputDotIndex = 0;
  gInputFreq = 0;
  ResetFreqInput();
}

void FINPUT_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed || bKeyHeld) {
    return;
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
    gRedrawScreen = true;
    break;
  case KEY_EXIT:
    if (freqInputIndex == 0) {
      APPS_run(previousApp);
      break;
    }
    input(key);
    gRedrawScreen = true;
    break;
  case KEY_MENU:
    tempFreq = GetTuneF(tempFreq);
    if (tempFreq >= F_MIN && tempFreq <= F_MAX) {
      gCurrentVfo.f = tempFreq;
    }
    APPS_run(previousApp);
    break;
  default:
    break;
  }
}

void FINPUT_render() {
  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
  UI_PrintString(freqInputString, 2, 127, 0, 8, true);
  UI_PrintStringSmallest(apps[previousApp].name, 0, 32, false, true);
}
