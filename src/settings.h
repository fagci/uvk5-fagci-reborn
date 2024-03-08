#ifndef SETTINGS_H
#define SETTINGS_H

#include "globals.h"
#include <stdint.h>

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
extern const uint8_t BL_TIME_VALUES[7];
extern const uint8_t EEPROM_CHECKBYTE;

void SETTINGS_Save();
void SETTINGS_Load();
void SETTINGS_DelayedSave();
uint32_t SETTINGS_GetFilterBound();
uint32_t SETTINGS_GetEEPROMSize();
uint8_t SETTINGS_GetPageSize();

#endif /* end of include guard: SETTINGS_H */
