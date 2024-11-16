#include "settings.h"
#include "driver/eeprom.h"
#include "scheduler.h"

Settings gSettings;
DwState gDW;

uint8_t BL_TIME_VALUES[7] = {0, 5, 10, 20, 60, 120, 255};
const char *BL_TIME_NAMES[7] = {"Off",  "5s",   "10s", "20s",
                                "1min", "2min", "On"};

const char *BL_SQL_MODE_NAMES[3] = {"Off", "On", "Open"};
const char *CH_DISPLAY_MODE_NAMES[3] = {"Name+F", "F", "Name"};
const char *TX_POWER_NAMES[4] = {"ULow", "Low", "Mid", "High"};
const char *TX_OFFSET_NAMES[3] = {"None", "+", "-"};
const char *TX_CODE_TYPES[4] = {"None", "CT", "DCS", "-DCS"};
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
const char *CH_TYPE_NAMES[8] = {
    "EMPTY", "CH", "PRESET", "VFO", "FOLDER", "MELODY", "SETTING", "FILE",
};

Settings defaultSettings = (Settings){
    .eepromType = EEPROM_BL24C64,
    .squelch = 4,
    .scrambler = 0,
    .batsave = 4,
    .vox = 0,
    .backlight = 3,
    .txTime = 0,
    .micGain = 15,
    .currentScanlist = 15,
    .roger = 0,
    .scanmode = 0,
    .chDisplayMode = 0,
    .dw = false,
    .crossBandScan = false,
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
    .scanTimeout = 50,
    .activeVFO = 0,
    .activePreset = 9,
    .backlightOnSquelch = BL_SQL_ON,
    .batteryCalibration = 2000,
    .batteryType = BAT_1600,
    .batteryStyle = BAT_PERCENT,
    .upconverter = 0,
};

const uint32_t EEPROM_SIZES[6] = {
    8192, 16384, 32768, 65536, 131072, 262144,
};

const uint16_t PAGE_SIZES[6] = {
    32, 64, 64, 128, 128, 128,
};

void SETTINGS_Save(void) {
  EEPROM_WriteBuffer(SETTINGS_OFFSET, &gSettings, SETTINGS_SIZE);
}

void SETTINGS_Load(void) {
  EEPROM_ReadBuffer(SETTINGS_OFFSET, &gSettings, SETTINGS_SIZE);
}

void SETTINGS_DelayedSave(void) {
  TaskRemove(SETTINGS_Save);
  TaskAdd("SetSav", SETTINGS_Save, 5000, false, 0);
}

uint32_t SETTINGS_GetFilterBound(void) {
  return gSettings.bound_240_280 ? VHF_UHF_BOUND2 : VHF_UHF_BOUND1;
}

uint32_t SETTINGS_GetEEPROMSize(void) {
  return EEPROM_SIZES[gSettings.eepromType];
}

uint16_t SETTINGS_GetPageSize(void) { return PAGE_SIZES[gSettings.eepromType]; }
