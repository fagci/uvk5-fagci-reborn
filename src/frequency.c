#include "frequency.h"
#include "globals.h"
#include "settings.h"

const FRange BAND_LPD = {43307500, 43477500};
const FRange BAND_PMR = {44600625, 44609375};
const FRange BAND_HAM2M = {14400000, 14799999};
const FRange BAND_HAM70CM = {43000000, 43999999};
const FRange BAND_SATCOM = {23000000, 31999999};

const FRange STOCK_BANDS[12] = {
    {1500000, 3000000},   {3000000, 5000000},    {5000000, 7600000},
    {10800000, 13500000}, {13600000, 17300000},  {17400000, 34900000},
    {35000000, 39900000}, {40000000, 46900000},  {47000000, 59900000},
    {60000000, 90000000}, {90000000, 120000000}, {120000000, 134000000},
};

const uint16_t StepFrequencyTable[12] = {
    1, 10, 100, 250, 500, 625, 833, 900, 1000, 1250, 2500, 10000,
};

const uint32_t upConverterValues[3] = {0, 5000000, 12500000};

uint32_t GetScreenF(uint32_t f) {
  return f - upConverterValues[gSettings.upconverter];
}

uint32_t GetTuneF(uint32_t f) {
  return f + upConverterValues[gSettings.upconverter];
}

bool FreqInRange(uint32_t f, const FRange *r) {
  return f >= r->start && f <= r->end;
}
