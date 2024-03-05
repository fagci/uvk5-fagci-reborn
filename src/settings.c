#include "settings.h"
#include "driver/eeprom.h"
#include "scheduler.h"

Settings gSettings;

uint8_t BL_TIME_VALUES[7] = {0, 5, 10, 20, 60, 120, 255};

const FRange STOCK_BANDS[12] = {
    {1500000, 3000000},   {3000000, 5000000},    {5000000, 7600000},
    {10800000, 13500000}, {13600000, 17300000},  {17400000, 34900000},
    {35000000, 39900000}, {40000000, 46900000},  {47000000, 59900000},
    {60000000, 90000000}, {90000000, 120000000}, {120000000, 134000000},
};

const uint32_t EEPROM_SIZES[8] = {
    8192,   // 000
    8192,   // 001
    8192,   // 010
    16384,  // 011
    32768,  // 100
    65536,  // 101
    131072, // 110
    262144, // 111
};

const uint8_t PAGE_SIZES[8] = {
    32,  // 000
    32,  // 001
    32,  // 010
    32,  // 011
    32,  // 100
    32,  // 101
    32,  // 110
    128, // 111
};

void SETTINGS_Save() {
  EEPROM_WriteBuffer(SETTINGS_OFFSET, &gSettings, SETTINGS_SIZE);
}

void SETTINGS_Load() {
  EEPROM_ReadBuffer(SETTINGS_OFFSET, &gSettings, SETTINGS_SIZE);
}

void SETTINGS_DelayedSave() {
  TaskRemove(SETTINGS_Save);
  TaskAdd("Settings save", SETTINGS_Save, 5000, false, 0);
}

uint32_t SETTINGS_GetFilterBound() {
  return gSettings.bound_240_280 ? VHF_UHF_BOUND2 : VHF_UHF_BOUND1;
}

uint32_t SETTINGS_GetEEPROMSize() {
  return EEPROM_SIZES[gSettings.eepromType];
}

uint8_t SETTINGS_GetPageSize() { return PAGE_SIZES[gSettings.eepromType]; }
