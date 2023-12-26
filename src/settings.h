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

typedef struct {
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
  AppType_t mainApp : 4;
  uint8_t reserved1 : 4;
  uint16_t activeChannel : 10;
  uint8_t activePreset : 6;
  uint8_t presetsCount : 8;
} __attribute__((packed)) Settings;

typedef struct {                 // 24 bytes
  uint32_t fRX;                  // 4
  uint32_t fTX;                  // 4
  char name[10];                 // 10
  uint8_t memoryBanks : 8;       // 1
  ModulationType modulation : 4; // 1
  BK4819_FilterBandwidth_t bw : 2;
  uint8_t power : 2;
  uint8_t codeTypeRx : 4;
  uint8_t codeTypeTx : 4; // 1
  uint8_t codeRx : 8;     // 1
  uint8_t codeTx : 8;     // 1
  uint8_t reserved : 8;   // 1
} __attribute__((packed)) VFO;

typedef struct { // 8 bytes
  uint32_t start;
  uint32_t end;
} __attribute__((packed)) FRange;

typedef struct { // 21 bytes
  FRange bounds;
  char name[10];
  Step step : 4;
  ModulationType modulation : 4;
  BK4819_FilterBandwidth_t bw : 2;
  SquelchType squelchType : 2;
  uint8_t squelch : 4;
  uint8_t gainIndex : 7;
  bool reserved1 : 1;
} __attribute__((packed)) Band;

typedef struct { // 29 bytes
  Band band;
  uint32_t offset;         // 4
  uint8_t memoryBanks : 8; // 1
  uint8_t codeTypeRx : 4;
  uint8_t codeTypeTx : 4; // 1
  uint8_t codeRx : 8;     // 1
  uint8_t codeTx : 8;     // 1
  uint8_t power : 2;
  bool allowTx : 1;
  uint8_t a : 5;
  uint8_t b : 8;
  uint8_t c : 8;
} __attribute__((packed)) Preset;

#define SETTINGS_OFFSET 0
#define SETTINGS_SIZE sizeof(Settings)

#define PRESET_SIZE sizeof(Preset)
#define VFO_SIZE sizeof(VFO)

#define BANDS_OFFSET sizeof(Settings)
#define CHANNELS_OFFSET 0x2000

extern Settings gSettings;
extern uint8_t BL_TIME_VALUES[7];
extern const char *BL_TIME_NAMES[7];

void SETTINGS_Save();
void SETTINGS_Load();
void SETTINGS_DelayedSave();

#endif /* end of include guard: SETTINGS_H */
