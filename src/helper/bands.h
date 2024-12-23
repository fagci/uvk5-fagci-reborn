#ifndef BANDS_H
#define BANDS_H

#include "channels.h"
#include <stdint.h>

typedef struct {
  uint32_t s;
  uint32_t e;
} SBand;

typedef struct {
  uint32_t s;
  uint32_t e;
  uint16_t mr;
  Step step;
} DBand;

typedef struct {
  uint32_t s;
  uint32_t e;
  PowerCalibration c;
} PCal;

void BANDS_Load();

uint8_t BANDS_DefaultCount();
Band BANDS_GetDefaultBand(uint8_t i);
PowerCalibration BANDS_GetPowerCalib(uint32_t f);

Band BANDS_Item(int8_t i);
int8_t BAND_IndexOf(Band p);
void BANDS_SelectBandRelative(bool next);
bool BANDS_SelectBandRelativeByScanlist(bool next);
void BAND_Select(int8_t i);
void BAND_SelectScan(int8_t i);
Band BAND_ByFrequency(uint32_t f);
bool BAND_SelectByFrequency(uint32_t f);
void BANDS_SaveCurrent();
bool BAND_InRange(const uint32_t f, const Band p);

extern Band defaultBand;
extern Band gCurrentBand;

#endif /* end of include guard: BANDS_H */
