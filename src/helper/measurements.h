#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include "../misc.h"
#include <stdint.h>

static const uint8_t rssi2s[2][15] = {
    {121, 115, 109, 103, 97, 91, 85, 79, 73, 63, 53, 43, 33, 23, 13},
    {141, 135, 129, 123, 117, 111, 105, 99, 93, 83, 73, 63, 53, 43, 33},
};

long long Clamp(long long v, long long min, long long max);
int ConvertDomain(int aValue, int aMin, int aMax, int bMin, int bMax);
uint8_t Rssi2PX(uint16_t rssi, uint8_t pxMin, uint8_t pxMax);
uint8_t DBm2S(int dbm, bool isVHF);
int Rssi2DBm(uint16_t rssi);
int Mid(uint16_t *array, uint8_t n);
int Min(uint16_t *array, uint8_t n);
int Max(uint16_t *array, uint8_t n);
uint16_t Mean(uint16_t *array, uint8_t n);
uint16_t Std(uint16_t *data, uint8_t n);
void IncDec8(uint8_t *val, uint8_t min, uint8_t max, int8_t inc);
void IncDecI8(int8_t *val, int8_t min, int8_t max, int8_t inc);
void IncDec16(uint16_t *val, uint16_t min, uint16_t max, int16_t inc);
void IncDecI16(int16_t *val, int16_t min, int16_t max, int16_t inc);
void IncDecI32(int32_t *val, int32_t min, int32_t max, int32_t inc);
void IncDec32(uint32_t *val, uint32_t min, uint32_t max, int32_t inc);
bool IsReadable(char *name);

#define INC_DEC(val, min, max, inc)                                            \
  do {                                                                         \
    if (inc > 0) {                                                             \
      val = ((((val - min) + inc) % (max - min + 1)) + (max - min + 1)) %      \
                (max - min + 1) +                                              \
            min;                                                               \
    } else if (inc < 0) {                                                      \
      val = ((((val - min) + inc) % (max - min + 1)) + (max - min + 1)) %      \
                (max - min + 1) +                                              \
            min;                                                               \
    }                                                                          \
  } while (0)

#endif /* end of include guard: MEASUREMENTS_H */
