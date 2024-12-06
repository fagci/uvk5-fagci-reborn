#ifndef SETTINGS_H
#define SETTINGS_H

#include "apps/apps.h"
#include "driver/bk4819.h"
#include <stdint.h>

#define getsize(V) char (*__ #V)(void)[sizeof(V)] = 1;

typedef struct {
  int8_t lastActiveVFO : 2;
  uint8_t activityOnVFO : 1; // activity on VFO#
  bool isSync : 1;           // do we have sync?
  bool doSync : 1;           // do sync in svc_listening service
  bool doSwitch : 1;         // do switch VFO
  bool doSwitchBack : 1;
} __attribute__((packed)) DwState;
extern DwState gDW;

typedef enum {
  STEP_0_02kHz,
  STEP_0_05kHz,
  STEP_0_5kHz,
  STEP_1_0kHz,

  STEP_2_5kHz,
  STEP_5_0kHz,
  STEP_6_25kHz,
  STEP_8_33kHz,
  STEP_9_0kHz,
  STEP_10_0kHz,
  STEP_12_5kHz,
  STEP_25_0kHz,
  STEP_50_0kHz,
  STEP_100_0kHz,
  STEP_500_0kHz,
} Step;

typedef enum {
  OFFSET_NONE,
  OFFSET_PLUS,
  OFFSET_MINUS,
  OFFSET_FREQ,
} OffsetDirection;

typedef enum {
  BL_SQL_OFF,
  BL_SQL_ON,
  BL_SQL_OPEN,
} BacklightOnSquelchMode;

typedef enum {
  CH_DISPLAY_MODE_NF,
  CH_DISPLAY_MODE_F,
  CH_DISPLAY_MODE_N,
} CHDisplayMode;

typedef enum {
  DW_OFF,
  DW_STAY,
  DW_SWITCH,
} DWType;

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
  TX_POW_ULOW,
  TX_POW_LOW,
  TX_POW_MID,
  TX_POW_HIGH,
} TXOutputPower;

typedef enum {
  RADIO_BK4819,
  RADIO_BK1080,
  RADIO_SI4732,
} Radio;

typedef enum {
  SCAN_TO_0,
  SCAN_TO_100ms,
  SCAN_TO_250ms,
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
  EEPROM_BL24C64,   //
  EEPROM_BL24C128,  //
  EEPROM_BL24C256,  //
  EEPROM_BL24C512,  //
  EEPROM_BL24C1024, //
  EEPROM_M24M02,    //
  EEPROM_UNKNOWN,
} EEPROMType;

extern const char *EEPROM_TYPE_NAMES[6];
extern const uint32_t EEPROM_SIZES[6];
extern const char *CH_TYPE_NAMES[8];

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
  uint8_t iAmPro : 1;
  uint8_t reserved2 : 1;
  uint8_t roger : 2;
  uint8_t scanmode : 2;
  CHDisplayMode chDisplayMode : 2;
  uint8_t pttLock : 1;
  uint8_t crossBandScan : 1;
  uint8_t beep : 1;
  uint8_t keylock : 1;
  uint8_t busyChannelTxLock : 1;
  uint8_t ste : 1;
  uint8_t repeaterSte : 1;
  uint8_t dtmfdecode : 1;
  uint8_t brightness : 4;
  uint8_t contrast : 4;
  AppType_t mainApp : 8;

  uint8_t activePreset : 8; // preset index
  uint16_t batteryCalibration : 12;
  BatteryType batteryType : 2;
  BatteryStyle batteryStyle : 2;
  ScanTimeout sqOpenedTimeout : 4;
  ScanTimeout sqClosedTimeout : 4;
  bool bound_240_280 : 1;
  bool noListen : 1;
  bool si4732PowerOff : 1;
  uint8_t dw : 2;
  bool toneLocal : 1;
  BacklightOnSquelchMode backlightOnSquelch : 2;
  uint8_t scanTimeout : 8;
  uint8_t sqlOpenTime : 3;
  uint8_t sqlCloseTime : 2;
  bool skipGarbageFrequencies : 1;
  uint8_t activeVFO : 2;
  uint32_t upconverter : 27;
} __attribute__((packed)) Settings;
// getsize(Settings)

typedef struct {
  uint32_t f : 27;

} __attribute__((packed)) F; // 5 B
// getsize(F)

typedef struct {
  uint8_t value;
  uint8_t type : 4;
} Code;

typedef struct {
  Code rx;
  Code tx;
} CodeRXTX;

typedef struct {
  uint32_t start : 27;
  uint32_t end : 27;
} __attribute__((packed)) FRange;

typedef struct {
  uint8_t s;
  uint8_t m;
  uint8_t e;
} __attribute__((packed)) PowerCalibration;

typedef struct {
  uint8_t value : 4;
  SquelchType type : 2;
} Squelch;

typedef enum {
  TYPE_EMPTY,
  TYPE_CH,
  TYPE_PRESET,
  TYPE_VFO,
  TYPE_FOLDER,
  TYPE_MELODY,
  TYPE_SETTING,
  TYPE_FILE,
} CHType;

typedef struct {
  CHType type : 3;
  bool readonly : 1;
} CHMeta;

typedef struct {
  // Common fields
  CHMeta meta;
  union {
    uint16_t scanlists;
    int16_t channel;
  };
  char name[10];
  union {
    struct {
      uint32_t rxF : 27;
      uint32_t txF : 27;
      OffsetDirection offsetDir : 2; // =0 -> tx=rxF
                                     // =1 -> tx=rxF+txF
                                     // =2 -> tx=rxF-txF
                                     // =4 -> tx=txF

      // Common radio settings
      Step step : 4;
      ModulationType modulation : 4;
      BK4819_FilterBandwidth_t bw : 4;
      Radio radio : 2;
      TXOutputPower power : 2;
      bool allowTx : 1;

      uint8_t scrambler : 4;
      Squelch squelch;

      union {
        // Only VFO/MR
        CodeRXTX code;

        // Only PRESET
        struct {
          uint8_t bank;
          PowerCalibration powCalib;
          uint32_t lastUsedFreq : 27;
        } misc;
      };

      uint8_t gainIndex : 5; // Common rest
    } __attribute__((packed));
  };
} __attribute__((packed)) CH;
// getsize(CH);
typedef CH Band;
typedef CH VFO;
typedef CH Preset;

#define SETTINGS_OFFSET (0)
#define SETTINGS_SIZE sizeof(Settings)

#define CH_SIZE sizeof(CH)
#define CHANNELS_OFFSET (SETTINGS_OFFSET + SETTINGS_SIZE)

// #define PATCH_SIZE 15832
#define PATCH_SIZE 8840

// settings
// VFOs
// channel 1
// channel 2

extern Settings gSettings;
extern bool isPatchPresent;
extern uint8_t BL_TIME_VALUES[7];
extern const char *BL_TIME_NAMES[7];
extern const char *BL_SQL_MODE_NAMES[3];
extern const char *CH_DISPLAY_MODE_NAMES[3];
extern const char *TX_POWER_NAMES[4];
extern const char *TX_OFFSET_NAMES[3];
extern const char *TX_CODE_TYPES[4];
extern const char *rogerNames[4];
extern const char *dwNames[3];

void SETTINGS_Save();
void SETTINGS_Load();
void SETTINGS_DelayedSave();
uint32_t SETTINGS_GetFilterBound();
uint32_t SETTINGS_GetEEPROMSize();
uint16_t SETTINGS_GetPageSize();
bool SETTINGS_IsPatchPresent();

#endif /* end of include guard: SETTINGS_H */
