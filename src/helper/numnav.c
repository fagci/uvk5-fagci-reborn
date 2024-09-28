#include "numnav.h"
#include <stddef.h>
#include <string.h>

bool gIsNumNavInput = false;
char gNumNavInput[16] = {'\0'};
void (*gNumNavCallback)(uint16_t result) = NULL;

static uint8_t pos = 0;
static uint16_t initV = 0;
static uint16_t minV = 0;
static uint16_t maxV = 0;
static uint8_t minDigits = 1;
static uint8_t maxDigits = 1;

void NUMNAV_Init(uint16_t initialValue, uint16_t min, uint16_t max) {
  initV = initialValue;
  minV = min;
  maxV = max;
  minDigits = 1;
  maxDigits = 1;
  while (min /= 10) {
    minDigits++;
  }
  while (max /= 10) {
    maxDigits++;
  }
  memset(gNumNavInput, '\0', 16);
  memset(gNumNavInput, '-', maxDigits);
  gIsNumNavInput = true;
}

uint16_t NUMNAV_GetCurrentValue(void) {
  if (pos == 0) {
    return initV;
  }
  uint16_t v = 0;
  uint16_t mul = 1;
  for (int8_t i = pos - 1; i >= 0; --i) {
    v += (gNumNavInput[i] - '0') * mul;
    mul *= 10;
  }
  return v;
}

uint16_t NUMNAV_Input(KEY_Code_t key) {
  if (pos == 0 && key == KEY_0) {
    NUMNAV_Deinit();
    return initV;
  }
  if (key == KEY_EXIT) {
    if (pos) {
      pos--;
      gNumNavInput[pos] = '-';
    } else {
      NUMNAV_Deinit();
    }
    return initV;
  }
  if (key == KEY_MENU || key == KEY_PTT) {
    NUMNAV_Accept();
    return NUMNAV_GetCurrentValue();
  }
  if (key > KEY_9) {
    uint16_t v = NUMNAV_GetCurrentValue();
    NUMNAV_Deinit();
    return v;
  }

  uint8_t nextNum = (uint8_t)key;
  gNumNavInput[pos] = '0' + nextNum;

  pos++;

  uint16_t v = NUMNAV_GetCurrentValue();
  if ((pos == maxDigits || v * 10 > maxV) && gNumNavCallback) {
    if (v >= minV && v <= maxV) {
      NUMNAV_Accept();
      return v;
    } else {
      return NUMNAV_Deinit();
    }
  }
  return NUMNAV_GetCurrentValue();
}

uint16_t NUMNAV_Deinit(void) {
  pos = 0;
  gNumNavCallback = NULL;
  gIsNumNavInput = false;
  return initV;
}

void NUMNAV_Accept(void) {
  gNumNavCallback(NUMNAV_GetCurrentValue());
  NUMNAV_Deinit();
}
