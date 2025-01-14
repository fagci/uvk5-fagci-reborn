#include "settings.h"
#include "driver/eeprom.h"
#include "scheduler.h"
#include <string.h>

DwState gDW;
bool isPatchPresent = false;

uint8_t BL_TIME_VALUES[7] = {0, 5, 10, 20, 60, 120, 255};
const char *BL_TIME_NAMES[7] = {"Off",  "5s",   "10s", "20s",
                                "1min", "2min", "On"};

const char *BL_SQL_MODE_NAMES[3] = {"Off", "On", "Open"};
const char *CH_DISPLAY_MODE_NAMES[3] = {"Name+F", "F", "Name"};
const char *rogerNames[4] = {"None", "Moto", "Tiny", "Call"};
const char *dwNames[3] = {"Off", "TX Stay", "TX Switch"};
const char *EEPROM_TYPE_NAMES[6] = {
    "BL24C64 #", // 010
    "BL24C128",  // 011
    "BL24C256",  // 100
    "BL24C512",  // 101
    "BL24C1024", // 110
    "M24M02",    // 111
};

/* static const uint8_t PATCH1_PREAMBLE[] = {0x15, 0x00, 0x0F, 0xE0,
                                          0xF2, 0x73, 0x76, 0x2F}; */
static const uint8_t PATCH3_PREAMBLE[] = {0x15, 0x00, 0x03, 0x74,
                                          0x0b, 0xd4, 0x84, 0x60};

Settings gSettings = (Settings){
    .eepromType = EEPROM_UNKNOWN,
    .batsave = 4,
    .vox = 0,
    .backlight = 3,
    .txTime = 0,
    .micGain = 15,
    .currentScanlist = SCANLIST_ALL,
    .roger = 0,
    .scanmode = 0,
    .chDisplayMode = 0,
    .dw = false,
    .beep = false,
    .keylock = false,
    .busyChannelTxLock = false,
    .ste = true,
    .repeaterSte = true,
    .dtmfdecode = false,
    .brightness = 8,
    .contrast = 8,
    .mainApp = APP_VFO2,
    .sqOpenedTimeout = SCAN_TO_NONE,
    .sqClosedTimeout = SCAN_TO_2s,
    .sqlOpenTime = 1,
    .sqlCloseTime = 1,
    .skipGarbageFrequencies = true,
    .scanTimeout = 5,
    .activeVFO = 0,
    .backlightOnSquelch = BL_SQL_ON,
    .batteryCalibration = 2000,
    .batteryType = BAT_1600,
    .batteryStyle = BAT_PERCENT,
    .upconverter = 0,
    .deviation = 130, // 1300
};

const uint32_t EEPROM_SIZES[6] = {
    8192, 16384, 32768, 65536, 131072, 262144,
};

const uint16_t PAGE_SIZES[6] = {
    32, 64, 64, 128, 128, 256,
};

void SETTINGS_Save(void) {
  EEPROM_WriteBuffer(SETTINGS_OFFSET, &gSettings, SETTINGS_SIZE);
}

void SETTINGS_Load(void) {
  EEPROM_ReadBuffer(SETTINGS_OFFSET, &gSettings, SETTINGS_SIZE);
}

void SETTINGS_DelayedSave(void) {
  TaskRemove(SETTINGS_Save);
  TaskAdd("SetSav", SETTINGS_Save, 1000, false, 0);
}

uint32_t SETTINGS_GetFilterBound(void) {
  return gSettings.bound_240_280 ? VHF_UHF_BOUND2 : VHF_UHF_BOUND1;
}

uint32_t SETTINGS_GetEEPROMSize(void) {
  return EEPROM_SIZES[gSettings.eepromType];
}

uint16_t SETTINGS_GetPageSize(void) { return PAGE_SIZES[gSettings.eepromType]; }

bool SETTINGS_IsPatchPresent() {
  if (SETTINGS_GetEEPROMSize() < 32768) {
    return false;
  }
  uint8_t buf[8];
  EEPROM_ReadBuffer(SETTINGS_GetEEPROMSize() - PATCH_SIZE, buf, 8);
  return memcmp(buf, PATCH3_PREAMBLE, 8) == 0;
}
