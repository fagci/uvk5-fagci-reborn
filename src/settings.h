#ifndef SETTINGS_H
#define SETTINGS_H

#include "apps/apps.h"
#include "driver/bk4819.h"
#include <stdint.h>

#define getsize(V) char (*__ #V)(void)[sizeof(V)] = 1;

extern const uint8_t EEPROM_CHECKBYTE;

typedef enum {
  STEP_0_01kHz,
  STEP_0_1kHz,
  STEP_0_5kHz,
  STEP_1_0kHz,

  STEP_2_5kHz,
  STEP_5_0kHz,
  STEP_6_25kHz,
  STEP_8_33kHz,
  STEP_10_0kHz,
  STEP_12_5kHz,
  STEP_25_0kHz,
  STEP_100_0kHz,
  STEP_125_0kHz,
  STEP_200_0kHz,
} Step;

typedef enum {
  UPCONVERTER_OFF,
  UPCONVERTER_50M,
  UPCONVERTER_125M,
} UpconverterTypes;

typedef enum {
  OFFSET_NONE,
  OFFSET_PLUS,
  OFFSET_MINUS,
} OffsetDirection;

typedef enum {
  BL_SQL_OFF,
  BL_SQL_ON,
  BL_SQL_OPEN,
} BacklightOnSquelchMode;

typedef enum {
  BAT_1600,
  BAT_2200,
  BAT_3500,
} BatteryType;

typedef enum {
  BAT_CLEAN,
  BAT_PERCENT,
  BAT_VOLTAGE,
} BatteryStyle;

typedef enum {
  TX_POW_LOW,
  TX_POW_MID,
  TX_POW_HIGH,
} TXOutputPower;

typedef enum {
  RADIO_BK4819,
  RADIO_BK1080,
  RADIO_SI4732,
  RADIO_UNKNOWN,
} Radio;

typedef enum {
  SCAN_TO_0,
  SCAN_TO_500ms,
  SCAN_TO_1s,
  SCAN_TO_2s,
  SCAN_TO_5s,
  SCAN_TO_10s,
  SCAN_TO_30s,
  SCAN_TO_1min,
  SCAN_TO_2min,
  SCAN_TO_5min,
  SCAN_TO_NONE,
} ScanTimeout;

typedef enum {
  EEPROM_A,         // 000
  EEPROM_B,         // 001
  EEPROM_BL24C64,   // 010 checkbyte default
  EEPROM_BL24C128,  // 011
  EEPROM_BL24C256,  // 100
  EEPROM_BL24C512,  // 101
  EEPROM_BL24C1024, // 110
  EEPROM_M24M02,    // 111
} EEPROMType;

extern const char *EEPROM_TYPE_NAMES[8];

typedef struct {
  EEPROMType eepromType : 3;
  uint8_t checkbyte : 5;
  uint8_t squelch : 4;
  uint8_t scrambler : 4;
  uint8_t batsave : 4;
  uint8_t vox : 4;
  uint8_t backlight : 4;
  uint8_t txTime : 4;
  uint8_t micGain : 4;
  uint8_t currentScanlist : 4;
  UpconverterTypes upconverter : 2;
  uint8_t roger : 2;
  uint8_t scanmode : 2;
  uint8_t chDisplayMode : 2;
  uint8_t dw : 1;
  uint8_t crossBand : 1;
  uint8_t beep : 1;
  uint8_t keylock : 1;
  uint8_t busyChannelTxLock : 1;
  uint8_t ste : 1;
  uint8_t repeaterSte : 1;
  uint8_t dtmfdecode : 1;
  uint8_t brightness : 4;
  uint8_t contrast : 4;
  AppType_t mainApp : 8;

  int8_t presetsCount : 8;
  int8_t activePreset : 8;
  uint16_t batteryCalibration : 12;
  BatteryType batteryType : 2;
  BatteryStyle batteryStyle : 2;
  ScanTimeout sqOpenedTimeout : 4;
  ScanTimeout sqClosedTimeout : 4;
  bool bound_240_280 : 1;
  bool noListen : 1;
  uint8_t reserved2 : 4;
  BacklightOnSquelchMode backlightOnSquelch : 2;
  uint8_t scanTimeout : 8;
  uint8_t sqlOpenTime : 3;
  uint8_t sqlCloseTime : 2;
  bool skipGarbageFrequencies : 1;
  uint8_t activeVFO : 2;
  char nickName[10];
} __attribute__((packed)) Settings;
// getsize(Settings)

typedef struct {
  uint32_t f : 27;
  uint8_t codeType : 4;
  uint8_t code;
} __attribute__((packed)) F; // 5 B
// getsize(F)

typedef struct {
  F rx;
  F tx;
  char name[10];
  uint8_t memoryBanks;
  ModulationType modulation : 4;
  BK4819_FilterBandwidth_t bw : 2;
  TXOutputPower power : 2;
  Radio radio : 2;
} __attribute__((packed)) CH; // 22 B
// getsize(CH)

typedef struct {
  F rx;
  F tx;
  int16_t channel;
  ModulationType modulation : 4;
  TXOutputPower power : 2;
  Radio radio : 2;
} __attribute__((packed)) VFO;
// getsize(VFO)

typedef struct {
  uint32_t start : 27;
  uint32_t end : 27;
} __attribute__((packed)) FRange;

typedef struct {
  FRange bounds;
  char name[10];
  Step step : 4;
  ModulationType modulation : 4;
  BK4819_FilterBandwidth_t bw : 2;
  SquelchType squelchType : 2;
  uint8_t squelch : 4;
  uint8_t gainIndex : 5;
  uint8_t reserved1 : 3;
} __attribute__((packed)) Band;

typedef struct {
  uint8_t s : 8;
  uint8_t m : 8;
  uint8_t e : 8;
} __attribute__((packed)) PowerCalibration;

typedef struct {
  PowerCalibration powCalib;
  uint32_t lastUsedFreq : 27;
  uint32_t offset : 26;
  Band band;
  uint8_t memoryBanks : 8;
  TXOutputPower power : 2;
  OffsetDirection offsetDir : 2;
  Radio radio : 2;
  bool allowTx : 1;
} __attribute__((packed)) Preset;
// getsize(Preset)

// char (*__chCount)(void)[(8196 - sizeof(Settings) - sizeof(Preset) * 29 -
// sizeof(VFO) * 2)/sizeof(CH)] = 1;

#define SETTINGS_OFFSET (0)
#define SETTINGS_SIZE sizeof(Settings)

#define PRESET_SIZE sizeof(Preset)
#define CH_SIZE sizeof(CH)
#define VFO_SIZE sizeof(VFO)

#define VFOS_OFFSET (SETTINGS_OFFSET + SETTINGS_SIZE)
#define PRESETS_OFFSET (VFOS_OFFSET + VFO_SIZE * 2)

#define PATCH_SIZE 15832

// settings
// VFOs
// presets
// ...
// channel 2
// channel 1

extern Settings gSettings;
extern uint8_t BL_TIME_VALUES[7];
extern const char *BL_TIME_NAMES[7];
extern const char *BL_SQL_MODE_NAMES[3];
extern const char *TX_POWER_NAMES[3];
extern const char *TX_OFFSET_NAMES[3];
extern const char *TX_CODE_TYPES[4];

void SETTINGS_Save();
void SETTINGS_Load();
void SETTINGS_DelayedSave();
uint32_t SETTINGS_GetFilterBound();
uint32_t SETTINGS_GetEEPROMSize();
uint16_t SETTINGS_GetPageSize();

#endif /* end of include guard: SETTINGS_H */
