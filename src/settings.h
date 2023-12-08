#ifndef SETTINGS_H
#define SETTINGS_H

#include "apps/apps.h"
#include "radio.h"
#include <stdint.h>

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
} __attribute__((packed)) Settings;

#define SETTINGS_SIZE sizeof(Settings)
#define SETTINGS_OFFSET 8128

extern Settings gSettings;
extern uint8_t BL_TIME_VALUES[7];
extern const char *BL_TIME_NAMES[7];

void SETTINGS_Save();
void SETTINGS_Load();
void SETTINGS_DelayedSave();

#endif /* end of include guard: SETTINGS_H */
