#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

typedef struct {           // 24 bytes
  uint32_t fRX : 32;       // 4
  uint32_t fTX : 32;       // 4
  char name[10];           // 10
  uint8_t memoryBanks : 8; // 1
  uint8_t step : 4;
  uint8_t modulation : 4; // 1
  uint8_t bw : 2;
  uint8_t power : 2;
  uint8_t codeTypeRx : 4; // 1
  uint8_t codeTypeTx : 4;
  uint8_t codeRx : 8; // 1
  uint8_t codeTx : 8; // 1
} VFO;

typedef struct {           // 28 bytes
  uint32_t fStart : 32;    // 4
  uint32_t fEnd : 32;      // 4
  uint32_t offset : 32;    // 4
  char name[10];           // 10
  uint8_t memoryBanks : 8; // 1
  uint8_t step : 4;
  uint8_t modulation : 4; // 1
  uint8_t bw : 2;
  uint8_t power : 2;
  uint8_t codeTypeRx : 4; // 1
  uint8_t codeTypeTx : 4;
  uint8_t codeRx : 8; // 1
  uint8_t codeTx : 8; // 1
  uint8_t squelch : 4;
  uint8_t squelchType : 2; // 1
} Band;

typedef struct {
  uint8_t squelch : 4;
  uint8_t scrambler : 4; // 1
  uint8_t batsave : 4;
  uint8_t vox : 4; // 1
  uint8_t backlight : 4;
  uint8_t txTime : 4; // 1
  uint8_t micGain : 4;
  uint8_t currentScanlist : 4; // 1
  uint8_t upconv : 2;
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
} Settings;

#define EEPROM_SIZE 8196

#define BANDS_COUNT 32
#define CHANNELS_COUNT 300

#define BAND_SIZE 28
#define VFO_SIZE 24
#define SETTINGS_SIZE 6

#define BANDS_OFFSET 0
#define CHANNELS_OFFSET (BANDS_OFFSET + BAND_SIZE * BANDS_COUNT)
#define CURRENT_VFO_OFFSET (CHANNELS_OFFSET + VFO_SIZE * CHANNELS_COUNT)
#define SETTINGS_OFFSET 8120

#define EEPROM_USAGE SETTINGS_OFFSET + SETTINGS_SIZE

#if EEPROM_USAGE > EEPROM_SIZE
#error Size exceeded
#endif

#endif /* end of include guard: SETTINGS_H */
