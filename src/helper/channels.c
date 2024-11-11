#include "channels.h"
#include "../driver/eeprom.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include <stddef.h>

int16_t gScanlistSize = 0;
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

static uint32_t GetChannelOffset(int16_t num) {
  return getChannelsEnd() - (num + 1) * CH_SIZE;
}

uint16_t CHANNELS_GetCountMax(void) {
  uint16_t n = (getChannelsEnd() - getChannelsStart()) / CH_SIZE;
  return n < SCANLIST_MAX ? n : SCANLIST_MAX;
}

void CHANNELS_Load(int16_t num, CH *p) {
  if (num >= 0) {
    EEPROM_ReadBuffer(GetChannelOffset(num), p, CH_SIZE);
  }
}

void CHANNELS_Save(int16_t num, CH *p) {
  if (num >= 0) {
    EEPROM_WriteBuffer(GetChannelOffset(num), p, CH_SIZE);
  }
}

void CHANNELS_Delete(int16_t num) {
  CH _ch = {
      .rxF = 0,
      .txF = 0,
      .name = {'\0'},
      .memoryBanks = 0,
      .modulation = MOD_FM,
      .bw = BK4819_FILTER_BW_14k,
      .power = TX_POW_LOW,
      .radio = RADIO_UNKNOWN,
  };
  CHANNELS_Save(num, &_ch);
}

bool CHANNELS_Existing(int16_t num) {
  if (num < 0 ||
      num >= CHANNELS_GetCountMax()) { // TODO: check, somewhere (CH scan, CH
                                       // list) getting larger than max CH
    return false;
  }
  char name[1] = {0};
  uint32_t addr = GetChannelOffset(num) + CH_NAME_OFFSET;
  EEPROM_ReadBuffer(addr, name, 1);
  return IsReadable(name);
}

uint8_t CHANNELS_Scanlists(int16_t num) {
  uint8_t scanlists;
  uint32_t addr = GetChannelOffset(num) + CH_BANKS_OFFSET;
  EEPROM_ReadBuffer(addr, &scanlists, 1);
  return scanlists;
}

int16_t CHANNELS_Next(int16_t base, bool next) {
  int16_t si = base;
  int16_t max = CHANNELS_GetCountMax();
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

void CHANNELS_LoadScanlist(uint8_t n) {
  gSettings.currentScanlist = n;
  uint8_t scanlistMask = 1 << n;
  gScanlistSize = 0;
  for (int16_t i = 0; i < CHANNELS_GetCountMax(); ++i) {
    if (!CHANNELS_Existing(i)) {
      continue;
    }
    if (n == 15 || (CHANNELS_Scanlists(i) & scanlistMask) == scanlistMask) {
      gScanlist[gScanlistSize] = i;
      gScanlistSize++;
    }
  }
  SETTINGS_Save();
}

void CHANNELS_LoadBlacklistToLoot() {
  uint8_t scanlistMask = 1 << 7;
  for (int16_t i = 0; i < CHANNELS_GetCountMax(); ++i) {
    if (!CHANNELS_Existing(i)) {
      continue;
    }
    if ((CHANNELS_Scanlists(i) & scanlistMask) == scanlistMask) {
      CH ch;
      CHANNELS_Load(i, &ch);
      Loot *loot = LOOT_AddEx(ch.rxF, true);
      loot->open = false;
      loot->blacklist = true;
      loot->lastTimeOpen = 0;
    }
  }
}
