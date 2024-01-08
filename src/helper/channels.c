#include "channels.h"
#include "../driver/eeprom.h"
#include "../driver/uart.h"
#include "../external/printf/printf.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "vfos.h"
#include <string.h>

uint16_t gScanlistSize = 0;
uint16_t gScanlist[350] = {0};

static uint16_t presetsSizeBytes() {
  return gSettings.presetsCount * PRESET_SIZE;
}

uint16_t CHANNELS_GetCountMax() {
  return (EEPROM_SIZE - PRESETS_OFFSET - presetsSizeBytes()) / CH_SIZE;
}

void CHANNELS_Load(uint16_t num, CH *p) {
  EEPROM_ReadBuffer(CHANNELS_OFFSET - (num + 1) * CH_SIZE, p, CH_SIZE);
}

void CHANNELS_Save(uint16_t num, CH *p) {
  EEPROM_WriteBuffer(CHANNELS_OFFSET - (num + 1) * CH_SIZE, p, CH_SIZE);
}

bool CHANNELS_Existing(uint16_t i) {
  char name[2] = {0};
  uint16_t addr = CHANNELS_OFFSET - ((i + 1) * CH_SIZE) + 4 + 4;
  EEPROM_ReadBuffer(addr, name, 1);
  return IsReadable(name);
}

uint8_t CHANNELS_Scanlists(uint16_t i) {
  uint8_t scanlists;
  uint16_t addr = CHANNELS_OFFSET - ((i + 1) * CH_SIZE) + 4 + 4 + 10;
  EEPROM_ReadBuffer(addr, &scanlists, 1);
  return scanlists;
}

int16_t CHANNELS_Next(int16_t base, bool next) {
  int16_t si = base;
  uint16_t max = CHANNELS_GetCountMax();
  IncDecI16(&si, 0, max, next ? 1 : -1);
  int16_t i = si;
  if (next) {
    for (; i < max; ++i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
    for (i = 0; i < base; ++i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
  } else {
    for (; i >= 0; --i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
    for (i = max - 1; i > base; --i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
  }
  return -1;
}

void CHANNELS_Delete(uint16_t i) {
  CH v = {0};
  CHANNELS_Save(i, &v);
}

void CHANNELS_LoadScanlist(uint8_t n) {
  gSettings.currentScanlist = n;
  uint16_t max = CHANNELS_GetCountMax();
  uint8_t scanlistMask = 1 << n;
  gScanlistSize = 0;
  for (uint16_t i = 0; i < max; ++i) {
    if ((n == 15 && CHANNELS_Existing(i)) ||
        (CHANNELS_Scanlists(i) & scanlistMask) == scanlistMask) {
      gScanlist[gScanlistSize] = i;
      gScanlistSize++;
    }
  }
  SETTINGS_Save();
}
