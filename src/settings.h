#ifndef SETTINGS_H
#define SETTINGS_H

#include "apps/apps.h"
#include "driver/bk4819.h"
#include <stdint.h>

#define getsize(V) char (*__ #V)(void)[sizeof(V)] = 1;

#define SCANLIST_ALL 0

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

// TODO: align fields
typedef struct {
  uint32_t upconverter : 27;
  uint8_t checkbyte : 5;

  uint16_t currentScanlist;
  AppType_t mainApp : 8;

  uint16_t batteryCalibration : 12;
  uint8_t contrast : 4;

  uint8_t backlight : 4;
  uint8_t reserved4 : 4;

  uint8_t reserved3 : 4;
  uint8_t batsave : 4;

  uint8_t vox : 4;
  uint8_t txTime : 4;

  uint8_t micGain : 4;
  uint8_t iAmPro : 1;
  bool reserved1 : 1;
  uint8_t roger : 2;

  uint8_t scanmode : 2;
  CHDisplayMode chDisplayMode : 2;
  uint8_t pttLock : 1;
  uint8_t reserved2 : 1;
  uint8_t beep : 1;
  uint8_t keylock : 1;

  uint8_t busyChannelTxLock : 1;
  uint8_t ste : 1;
  uint8_t repeaterSte : 1;
  uint8_t dtmfdecode : 1;
  uint8_t brightness : 4;

  uint8_t brightnessLow : 4;
  EEPROMType eepromType : 3;
  bool bound_240_280 : 1;

  ScanTimeout sqClosedTimeout : 4;
  ScanTimeout sqOpenedTimeout : 4;

  BatteryType batteryType : 2;
  BatteryStyle batteryStyle : 2;
  bool noListen : 1;
  bool si4732PowerOff : 1;
  uint8_t dw : 2;

  uint8_t scanTimeout;

  BacklightOnSquelchMode backlightOnSquelch : 2;
  bool toneLocal : 1;
  uint8_t sqlOpenTime : 3;
  uint8_t sqlCloseTime : 2;

  uint8_t deviation;

  uint8_t activeVFO : 2;
  bool skipGarbageFrequencies : 1;

} __attribute__((packed)) Settings;
// getsize(Settings)

#define SETTINGS_OFFSET (0)
#define SETTINGS_SIZE sizeof(Settings)

#define CH_SIZE sizeof(CH)
#define CHANNELS_OFFSET (SETTINGS_OFFSET + SETTINGS_SIZE)

#define PATCH1_SIZE 15832
#define PATCH3_SIZE 8840

#define PATCH_SIZE PATCH3_SIZE

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
