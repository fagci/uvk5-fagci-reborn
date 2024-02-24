#ifndef SETTINGS_H
#define SETTINGS_H

#include "apps/apps.h"
#include "driver/bk4819.h"
#include <stdint.h>

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
  uint8_t scrambler : 4; // 1
  uint8_t batsave : 4;
  uint8_t vox : 4; // 1
  uint8_t backlight : 4;
  uint8_t txTime : 4; // 1
  uint8_t micGain : 4;
  uint8_t currentScanlist : 4; // 1
  UpconverterTypes upconverter : 2;
  uint8_t roger : 2;
  uint8_t scanmode : 2;
  uint8_t chDisplayMode : 2; // 1
  uint8_t dw : 1;
  uint8_t crossBand : 1;
  uint8_t beep : 1;
  uint8_t keylock : 1;
  uint8_t busyChannelTxLock : 1;
  uint8_t ste : 1;
  uint8_t repeaterSte : 1;
  uint8_t dtmfdecode : 1; // 1
  uint8_t brightness : 4;
  uint8_t contrast : 4;  // 1
  AppType_t mainApp : 8; // 1

  int8_t presetsCount : 8; // 1
  int8_t activePreset : 8; // 2
  uint16_t batteryCalibration : 12;
  BatteryType batteryType : 2;
  BatteryStyle batteryStyle : 2; // 2
  ScanTimeout sqOpenedTimeout : 4;
  ScanTimeout sqClosedTimeout : 4; // 1
  bool bound_240_280 : 1;
  bool noListen : 1;
  uint8_t reserved2 : 4;
  BacklightOnSquelchMode backlightOnSquelch : 2;
  uint8_t scanTimeout : 8;
  uint8_t sqlOpenTime : 2;
  uint8_t sqlCloseTime : 3;
  bool skipGarbageFrequencies : 1;
  uint8_t activeVFO : 2;
  char nickName[10];
} __attribute__((packed)) Settings;

typedef struct {           // 24 bytes
  uint32_t fRX;            // 4
  uint32_t fTX;            // 4
  char name[10];           // 10
  uint8_t memoryBanks : 8; // 1
  ModulationType modulation : 4;
  BK4819_FilterBandwidth_t bw : 2;
  TXOutputPower power : 2;
  uint8_t codeRx : 8; // 1
  uint8_t codeTx : 8; // 1
  uint8_t codeTypeRx : 4;
  uint8_t codeTypeTx : 4; // 1
  uint8_t reserved : 6;   // 1
} __attribute__((packed)) CH;

typedef struct { // 24 bytes
  uint32_t fRX;  // 4
  uint32_t fTX;  // 4
  uint8_t reserved00 : 4;
  bool reserved01 : 1;
  bool isMrMode : 1;
  TXOutputPower power : 2;
  uint8_t codeRx : 8; // 1
  uint8_t codeTx : 8; // 1
  uint8_t codeTypeRx : 4;
  uint8_t codeTypeTx : 4; // 1
  int32_t channel : 32;   // 1
} __attribute__((packed)) VFO;

typedef struct { // 8 bytes
  uint32_t start;
  uint32_t end;
} __attribute__((packed)) FRange;

typedef struct { // 21 bytes
  FRange bounds; // 8
  char name[10]; // 18
  Step step : 4;
  ModulationType modulation : 4; // 19
  BK4819_FilterBandwidth_t bw : 2;
  SquelchType squelchType : 2;
  uint8_t squelch : 4; // 20
  uint8_t gainIndex : 5;
  uint8_t reserved1 : 3; // 21
} __attribute__((packed)) Band;

typedef struct {
  uint8_t s;
  uint8_t m;
  uint8_t e;
} __attribute__((packed)) PowerCalibration;

typedef struct { // 29 bytes
  Band band;     // 21
  uint32_t offset : 30;
  OffsetDirection offsetDir : 2; // 25
  uint8_t memoryBanks : 8;       // 26
  uint8_t codeTypeRx : 4;
  uint8_t codeTypeTx : 4; // 27
  uint8_t codeRx : 8;     // 28
  uint8_t codeTx : 8;     // 29
  TXOutputPower power : 2;
  bool allowTx : 1;
  uint8_t a : 5;             // 30
  PowerCalibration powCalib; // 33
  uint32_t lastUsedFreq;
} __attribute__((packed)) Preset;

#define SETTINGS_OFFSET (0)
#define SETTINGS_SIZE sizeof(Settings)

#define PRESET_SIZE sizeof(Preset)
#define CH_SIZE sizeof(CH)
#define VFO_SIZE sizeof(VFO)

#define VFOS_OFFSET (SETTINGS_OFFSET + SETTINGS_SIZE)
#define PRESETS_OFFSET (VFOS_OFFSET + VFO_SIZE * 2)

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

void SETTINGS_Save();
void SETTINGS_Load();
void SETTINGS_DelayedSave();
uint32_t SETTINGS_GetFilterBound();
uint32_t SETTINGS_GetEEPROMSize();

#endif /* end of include guard: SETTINGS_H */
