#ifndef SETTINGS_H
#define SETTINGS_H

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

typedef enum {
  CH_CHANNEL,
  CH_VFO,
  CH_PRESET,
} ChannelType;

typedef enum {
  TX_DISALLOW,
  TX_ALLOW_LPD_PMR,
  TX_ALLOW_LPD_PMR_SATCOM,
  TX_ALLOW_HAM,
  TX_ALLOW_ALL,
} AllowTX;


typedef struct {
  uint8_t timeout : 8;
  ScanTimeout openedTimeout : 4;
  ScanTimeout closedTimeout : 4;
} __attribute__((packed)) ScanSettings;

typedef struct {
  uint8_t level : 6;
  uint8_t openTime : 2;
  SquelchType type;
  uint8_t closeTime : 3;
} __attribute__((packed)) SquelchSettings;
// getsize(SquelchSettings)

typedef struct {
  uint32_t start : 27;
  uint32_t end : 27;
} __attribute__((packed)) FRange;

typedef struct {
  uint8_t s : 8;
  uint8_t m : 8;
  uint8_t e : 8;
} __attribute__((packed)) PowerCalibration;

typedef struct {
  int16_t channel;
  ScanSettings scan;
} VFO_Params;

typedef struct {
  EEPROMType eepromType : 3;
  uint8_t checkbyte : 5; // 1
  uint8_t scrambler : 4;
  uint8_t batsave : 4; // 2
  uint8_t vox : 4;
  uint8_t backlight : 4; // 3
  uint8_t txTime : 4;
  uint8_t micGain : 4; // 4
  uint8_t currentScanlist : 4;
  UpconverterTypes upconverter : 2;
  uint8_t roger : 2; // 5
  uint8_t scanmode : 2;
  uint8_t chDisplayMode : 2;
  uint8_t dw : 1;
  uint8_t crossBand : 1;
  uint8_t beep : 1;
  uint8_t keylock : 1; // 6
  uint8_t busyChannelTxLock : 1;
  uint8_t ste : 1;
  uint8_t repeaterSte : 1;
  uint8_t dtmfdecode : 1;
  uint8_t brightness : 4; // 7
  uint8_t mainApp : 8;    // 8

  int8_t bandsCount : 8; // 9
  int8_t activeBand : 8; // 10
  uint16_t batteryCalibration : 12;
  uint8_t contrast : 4; // 12
  BatteryType batteryType : 2;
  BatteryStyle batteryStyle : 2;
  bool bound_240_280 : 1;
  bool noListen : 1;
  BacklightOnSquelchMode backlightOnSquelch : 2; // 13
  uint8_t reserved2 : 5;
  bool skipGarbageFrequencies : 1;
  uint8_t activeCH : 2;
  char nickName[10];
  PowerCalibration powCalib[12];
  AllowTX allowTX;
} __attribute__((packed)) Settings;
// getsize(Settings)

typedef struct {
  ChannelType type : 2;
  union {
    char name[10];
    VFO_Params vfo;
  };
  uint32_t f : 27;
  uint32_t offset : 27;
  OffsetDirection offsetDir;
  ModulationType modulation : 4;
  BK4819_FilterBandwidth_t bw : 2;
  TXOutputPower power : 2;
  uint8_t codeRX;
  uint8_t codeTX;
  uint8_t codeTypeRX : 4;
  uint8_t codeTypeTX : 4;
  uint8_t scanlists;
  SquelchSettings sq;
  uint8_t gainIndex : 5;
  Step step : 4;
} __attribute__((packed)) CH; // 33 B

// getsize(CH);

typedef struct {
  char name[10];
  uint8_t activeCH;
} __attribute__((packed)) Scanlist;

#define SETTINGS_SIZE sizeof(Settings)
#define BAND_SIZE sizeof(Band)
#define SCANLIST_SIZE sizeof(Scanlist)
#define CH_SIZE sizeof(CH)

#define SETTINGS_OFFSET (0)
#define SCANLISTS_OFFSET (SETTINGS_OFFSET + SETTINGS_SIZE)
#define BANDS_OFFSET (SCANLISTS_OFFSET + SCANLIST_SIZE * 8)

// settings
// CHs
// bands
// ...
// channel 2
// channel 1

extern Settings gSettings;
extern uint8_t BL_TIME_VALUES[7];

extern const FRange STOCK_BANDS[12];

void SETTINGS_Save();
void SETTINGS_Load();
void SETTINGS_DelayedSave();
uint32_t SETTINGS_GetFilterBound();
uint32_t SETTINGS_GetEEPROMSize();
uint8_t SETTINGS_GetPageSize();

#endif /* end of include guard: SETTINGS_H */
