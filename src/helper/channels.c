#include "channels.h"
#include "../driver/eeprom.h"
#include "../helper/measurements.h"
#include "../settings.h"
#include <stddef.h>

int16_t gScanlistSize = 0;
int32_t gScanlist[350] = {0};

CH vfos[10] = {0};
uint8_t vfosCount = 0;

int16_t CHANNELS_GetCountMax() {
  return (SETTINGS_GetEEPROMSize() - CHANNELS_END_OFFSET) / CH_SIZE;
}

static uint32_t getChOffset(int16_t num) {
  return SETTINGS_GetEEPROMSize() - (num + 1) * CH_SIZE;
}

void CHANNELS_Load(int16_t num, CH *p) {
  if (num >= 0) {
    EEPROM_ReadBuffer(getChOffset(num), p, CH_SIZE);
  }
}

void CHANNELS_Save(int16_t num, CH *p) {
  if (num >= 0) {
    EEPROM_WriteBuffer(getChOffset(num), p, CH_SIZE);
  }
}

bool CHANNELS_Existing(int16_t i) {
  ChannelType type;
  EEPROM_ReadBuffer(getChOffset(i), &type, 1);
  return type != CH_EMPTY;
}

ChannelType CHANNELS_GetType(int16_t i) {
  ChannelType type;
  EEPROM_ReadBuffer(getChOffset(i), &type, 1);
  return type;
}

uint8_t CHANNELS_Scanlists(int16_t i) {
  uint8_t groups;
  uint32_t addr = getChOffset(i) + offsetof(CH, groups);
  EEPROM_ReadBuffer(addr, &groups, 1);
  return groups;
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

void CHANNELS_Delete(int16_t i) {
  CH v = {.type = 255};
  CHANNELS_Save(i, &v);
}

void CHANNELS_LoadScanlist(uint8_t n) {
  gSettings.currentScanlist = n;
  int32_t max = CHANNELS_GetCountMax();
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

void VFO_LoadAll() {
  for (int16_t i = 0; i < CHANNELS_GetCountMax(); ++i) {
    if (CHANNELS_GetType(i) == CH_VFO) {
      CHANNELS_Load(i, &vfos[vfosCount]);

      App *app = APPS_GetById(vfos[vfosCount].vfo.app);
      AppVFOSlots *slots = app->vfoSlots;
      if (slots && slots->count < slots->maxCount) {
        slots->slots[slots->count++] = vfos[vfosCount];
      }

      vfosCount++;
    }
  }
}
