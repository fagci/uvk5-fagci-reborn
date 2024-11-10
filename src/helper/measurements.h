#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include "../misc.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint16_t ro;
  uint16_t rc;
  uint8_t no;
  uint8_t nc;
  uint8_t go;
  uint8_t gc;
} SQL;

static const uint8_t rssi2s[2][15] = {
    {121, 115, 109, 103, 97, 91, 85, 79, 73, 63, 53, 43, 33, 23, 13},
    {141, 135, 129, 123, 117, 111, 105, 99, 93, 83, 73, 63, 53, 43, 33},
};

long long Clamp(long long v, long long min, long long max);
int ConvertDomain(int aValue, int aMin, int aMax, int bMin, int bMax);
uint32_t ClampF(uint32_t v, uint32_t min, uint32_t max);
uint32_t ConvertDomainF(uint32_t aValue, uint32_t aMin, uint32_t aMax,
                        uint32_t bMin, uint32_t bMax);
uint8_t Rssi2PX(uint16_t rssi, uint8_t pxMin, uint8_t pxMax);
uint8_t DBm2S(int dbm, bool isVHF);
int Rssi2DBm(uint16_t rssi);
uint16_t Mid(const uint16_t *array, uint8_t n);
uint16_t Min(const uint16_t *array, uint8_t n);
uint16_t Max(const uint16_t *array, uint8_t n);
uint16_t Mean(const uint16_t *array, uint8_t n);
uint16_t Std(const uint16_t *data, uint8_t n);
void IncDec8(uint8_t *val, uint8_t min, uint8_t max, int8_t inc);
void IncDecI8(int8_t *val, int8_t min, int8_t max, int8_t inc);
void IncDec16(uint16_t *val, uint16_t min, uint16_t max, int16_t inc);
void IncDecI16(int16_t *val, int16_t min, int16_t max, int16_t inc);
void IncDecI32(int32_t *val, int32_t min, int32_t max, int32_t inc);
void IncDec32(uint32_t *val, uint32_t min, uint32_t max, int32_t inc);
bool IsReadable(char *name);
SQL GetSql(uint8_t level);

#endif /* end of include guard: MEASUREMENTS_H */
