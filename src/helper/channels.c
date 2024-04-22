#include "channels.h"
#include "../driver/eeprom.h"
#include "../helper/measurements.h"
#include <stddef.h>

int32_t gScanlistSize = 0;
int32_t gScanlist[350] = {0};

static uint16_t presetsSizeBytes(void) {
  return gSettings.presetsCount * PRESET_SIZE;
}

static uint32_t getChannelsEnd() {
  return SETTINGS_GetEEPROMSize() - PATCH_SIZE;
}

int32_t CHANNELS_GetCountMax(void) {
  return (getChannelsEnd() - PRESETS_OFFSET - presetsSizeBytes()) / CH_SIZE;
}

void CHANNELS_Load(int32_t num, CH *p) {
  if (num >= 0) {
    EEPROM_ReadBuffer(getChannelsEnd() - (num + 1) * CH_SIZE, p, CH_SIZE);
  }
}

void CHANNELS_Save(int32_t num, CH *p) {
  if (num >= 0) {
    EEPROM_WriteBuffer(getChannelsEnd() - (num + 1) * CH_SIZE, p, CH_SIZE);
  }
}

bool CHANNELS_Existing(int32_t i) {
  char name[1] = {0};
  uint32_t addr = getChannelsEnd() - ((i + 1) * CH_SIZE) + offsetof(CH, name);
  EEPROM_ReadBuffer(addr, name, 1);
  return IsReadable(name);
}

uint8_t CHANNELS_Scanlists(int32_t i) {
  uint8_t scanlists;
  uint32_t addr =
      getChannelsEnd() - ((i + 1) * CH_SIZE) + offsetof(CH, memoryBanks);
  EEPROM_ReadBuffer(addr, &scanlists, 1);
  return scanlists;
}

int32_t CHANNELS_Next(int32_t base, bool next) {
  int32_t si = base;
  int32_t max = CHANNELS_GetCountMax();
  IncDecI32(&si, 0, max, next ? 1 : -1);
  int32_t i = si;
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

void CHANNELS_Delete(int32_t i) {
  CH v = {0};
  CHANNELS_Save(i, &v);
}

void CHANNELS_LoadScanlist(uint8_t n) {
  gSettings.currentScanlist = n;
  int32_t max = CHANNELS_GetCountMax();
  max = 128; // temporary
  uint8_t scanlistMask = 1 << n;
  gScanlistSize = 0;
  for (int32_t i = 0; i < max; ++i) {
    if ((n == 15 && CHANNELS_Existing(i)) ||
        (CHANNELS_Scanlists(i) & scanlistMask) == scanlistMask) {
      gScanlist[gScanlistSize] = i;
      gScanlistSize++;
    }
  }
  SETTINGS_Save();
}
