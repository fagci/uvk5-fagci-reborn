#ifndef SETTINGS_H
#define SETTINGS_H

#include "radio.h"
#include <stdint.h>

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
} Settings;

#define SETTINGS_SIZE 7
#define SETTINGS_OFFSET 8120

extern Settings gSettings;

void SETTINGS_Save();
void SETTINGS_Load();

#endif /* end of include guard: SETTINGS_H */
