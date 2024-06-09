#include "channels.h"
#include "../driver/eeprom.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include <stddef.h>

#define SCANLIST_MAX 128

int32_t gScanlistSize = 0;
uint16_t gScanlist[SCANLIST_MAX] = {0};

static const uint8_t CH_NAME_OFFSET = offsetof(CH, name);
static const uint8_t CH_BANKS_OFFSET = offsetof(CH, memoryBanks);

static uint32_t presetsSizeBytes(void) {
  return ARRAY_SIZE(defaultPresets) * PRESET_SIZE;
}

static uint32_t getChannelsStart() {
  return PRESETS_OFFSET + presetsSizeBytes();
}

static uint32_t getChannelsEnd() {
  uint32_t eepromSize = SETTINGS_GetEEPROMSize();
  uint32_t minSizeWithPatch = getChannelsStart() + CH_SIZE + PATCH_SIZE;
  if (eepromSize < minSizeWithPatch) {
    return eepromSize;
  }
  return eepromSize - PATCH_SIZE;
}

static uint32_t GetChannelOffset(int32_t num) {
  return getChannelsEnd() - (num + 1) * CH_SIZE;
}

uint16_t CHANNELS_GetCountMax(void) {
  return (getChannelsEnd() - getChannelsStart()) / CH_SIZE;
}

void CHANNELS_Load(int32_t num, CH *p) {
  if (num >= 0) {
    EEPROM_ReadBuffer(GetChannelOffset(num), p, CH_SIZE);
  }
}

void CHANNELS_Save(int32_t num, CH *p) {
  if (num >= 0) {
    EEPROM_WriteBuffer(GetChannelOffset(num), p, CH_SIZE);
  }
}

void CHANNELS_Delete(int32_t num) {
  char name[1] = {0};
  uint32_t addr = GetChannelOffset(num) + CH_NAME_OFFSET;
  EEPROM_WriteBuffer(addr, name, 1);
}

bool CHANNELS_Existing(int32_t num) {
  if (num < 0) {
    return false;
  }
  char name[1] = {0};
  uint32_t addr = GetChannelOffset(num) + CH_NAME_OFFSET;
  EEPROM_ReadBuffer(addr, name, 1);
  return IsReadable(name);
}

uint8_t CHANNELS_Scanlists(int32_t num) {
  uint8_t scanlists;
  uint32_t addr = GetChannelOffset(num) + CH_BANKS_OFFSET;
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

void CHANNELS_LoadScanlist(uint8_t n) {
  gSettings.currentScanlist = n;
  int32_t max = CHANNELS_GetCountMax();
  max = 1024; // temporary
  uint8_t scanlistMask = 1 << n;
  gScanlistSize = 0;
  for (int32_t i = 0; i < max; ++i) {
    if (!CHANNELS_Existing(i)) {
      continue;
    }
    if (n == 15 || (CHANNELS_Scanlists(i) & scanlistMask) == scanlistMask) {
      gScanlist[gScanlistSize] = i;
      gScanlistSize++;
      if (gScanlistSize >= SCANLIST_MAX) {
        break;
      }
    }
  }
  SETTINGS_Save();
}
