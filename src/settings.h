#ifndef SETTINGS_H
#define SETTINGS_H

#include "apps/apps.h"
#include "driver/bk4819.h"
#include <stdint.h>

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
  BL_SQL_OFF,
  BL_SQL_ON,
  BL_SQL_OPEN,
} BacklightOnSquelchMode;

typedef enum {
  BAT_1600,
  BAT_2200,
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

typedef struct {
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

  uint8_t presetsCount : 8; // 1
  BacklightOnSquelchMode backlightOnSquelch : 2;
  uint8_t activePreset : 6; // 2
  uint16_t batteryCalibration : 12;
  BatteryType batteryType : 2;
  BatteryStyle batteryStyle : 2; // 2
  bool bound_240_280 : 1;
  ScanTimeout sqOpenedTimeout : 4;
  ScanTimeout sqClosedTimeout : 4;
  uint8_t reserved2 : 7;
  uint8_t reserved3 : 8;
  uint8_t reserved4 : 6;
  uint8_t activeVFO : 2;
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
  uint8_t reserved01 : 2;
  TXOutputPower power : 2;
  uint8_t codeRx : 8; // 1
  uint8_t codeTx : 8; // 1
  uint8_t codeTypeRx : 4;
  uint8_t codeTypeTx : 4; // 1
  int16_t channel : 12;   // 1
  bool isMrMode : 1;
  uint8_t reserved0 : 3;
  uint8_t reserved1 : 8;
  uint8_t reserved2 : 8;
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
  uint8_t gainIndex : 7;
  bool reserved1 : 1; // 21
} __attribute__((packed)) Band;

typedef struct {
  uint8_t s;
  uint8_t m;
  uint8_t e;
} PowerCalibration;

typedef struct {           // 29 bytes
  Band band;               // 21
  uint32_t offset;         // 25
  uint8_t memoryBanks : 8; // 26
  uint8_t codeTypeRx : 4;
  uint8_t codeTypeTx : 4; // 27
  uint8_t codeRx : 8;     // 28
  uint8_t codeTx : 8;     // 29
  TXOutputPower power : 2;
  bool allowTx : 1;
  uint8_t a : 5;             // 30
  PowerCalibration powCalib; // 33
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

void SETTINGS_Save();
void SETTINGS_Load();
void SETTINGS_DelayedSave();
uint32_t SETTINGS_GetFilterBound();

#endif /* end of include guard: SETTINGS_H */
