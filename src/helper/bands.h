#ifndef BANDS_H
#define BANDS_H

#include "channels.h"
#include <stdint.h>

#define BANDS_COUNT_MAX 70

typedef struct {
  uint32_t s;
  uint32_t e;
} SBand;

typedef struct {
  uint32_t s;
  uint32_t e;
  uint16_t mr;
  Step step; // needed to select band by freq
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

bool BANDS_SelectBandRelativeByScanlist(bool next);
void BANDS_SelectScan(int8_t i);
Band BANDS_ByFrequency(uint32_t f);
bool BANDS_SelectByFrequency(uint32_t f, bool copyToVfo);
void BANDS_SaveCurrent();
bool BANDS_InRange(const uint32_t f, const Band p);
uint8_t BANDS_GetScanlistIndex();
void BANDS_Select(int16_t num, bool copyToVfo);

extern Band defaultBand;
extern Band gCurrentBand;

#endif /* end of include guard: BANDS_H */
