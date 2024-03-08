#ifndef FREQUENCY_H
#define FREQUENCY_H

#include <stdint.h>

typedef struct {
  uint32_t start : 27;
  uint32_t end : 27;
} __attribute__((packed)) FRange;

uint32_t GetScreenF(uint32_t f);
uint32_t GetTuneF(uint32_t f);
bool FreqInRange(uint32_t f, const FRange *r);

extern const FRange STOCK_BANDS[12];

extern const FRange BAND_LPD;
extern const FRange BAND_PMR;
extern const FRange BAND_HAM2M;
extern const FRange BAND_HAM70CM;
extern const FRange BAND_SATCOM;

#endif /* end of include guard: FREQUENCY_H */
