#include "../helper/measurements.h"
#include <stdint.h>

long long Clamp(long long v, long long min, long long max) {
  return v <= min ? min : (v >= max ? max : v);
}

int ConvertDomain(int aValue, int aMin, int aMax, int bMin, int bMax) {
  const int aRange = aMax - aMin;
  const int bRange = bMax - bMin;
  aValue = Clamp(aValue, aMin, aMax);
  return ((aValue - aMin) * bRange + aRange / 2) / aRange + bMin;
}

uint32_t ClampF(uint32_t v, uint32_t min, uint32_t max) {
  return v <= min ? min : (v >= max ? max : v);
}

uint32_t ConvertDomainF(uint32_t aValue, uint32_t aMin, uint32_t aMax,
                        uint32_t bMin, uint32_t bMax) {
  const uint64_t aRange = aMax - aMin;
  const uint64_t bRange = bMax - bMin;
  aValue = ClampF(aValue, aMin, aMax);
  return ((aValue - aMin) * bRange + aRange / 2) / aRange + bMin;
}

uint8_t DBm2S(int dbm, bool isVHF) {
  uint8_t i = 0;
  dbm *= -1;
  for (i = 0; i < 15; i++) {
    if (dbm >= rssi2s[isVHF][i]) {
      return i;
    }
  }
  return i;
}

int Rssi2DBm(uint16_t rssi) { return (rssi >> 1) - 160; }

// applied x2 to prevent initial rounding
uint8_t Rssi2PX(uint16_t rssi, uint8_t pxMin, uint8_t pxMax) {
  return ConvertDomain(rssi - 320, -260, -120, pxMin, pxMax);
}

uint16_t Mid(const uint16_t *array, uint8_t n) {
  int32_t sum = 0;
  for (uint8_t i = 0; i < n; ++i) {
    sum += array[i];
  }
  return sum / n;
}

uint16_t Min(const uint16_t *array, uint8_t n) {
  uint16_t min = array[0];
  for (uint8_t i = 1; i < n; ++i) {
    if (array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

uint16_t Max(const uint16_t *array, uint8_t n) {
  uint16_t max = array[0];
  for (uint8_t i = 1; i < n; ++i) {
    if (array[i] > max) {
      max = array[i];
    }
  }
  return max;
}

uint16_t Mean(const uint16_t *array, uint8_t n) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < n; ++i) {
    sum += array[i];
  }
  return sum / n;
}

uint16_t Sqrt(uint32_t v) {
  uint16_t res = 0;
  for (uint32_t i = 0; i < v; ++i) {
    if (i * i <= v) {
      res = i;
    } else {
      break;
    }
  }
  return res;
}

uint16_t Std(const uint16_t *data, uint8_t n) {
  uint32_t sumDev = 0;

  for (uint8_t i = 0; i < n; ++i) {
    sumDev += data[i] * data[i];
  }
  return Sqrt(sumDev / n);
}

void IncDec8(uint8_t *val, uint8_t min, uint8_t max, int8_t inc) {
  if (inc > 0) {
    *val = *val == max - inc ? min : *val + inc;
  } else {
    *val = *val > min ? *val + inc : max + inc;
  }
}

void IncDecI8(int8_t *val, int8_t min, int8_t max, int8_t inc) {
  if (inc > 0) {
    *val = *val == max - inc ? min : *val + inc;
  } else {
    *val = *val > min ? *val + inc : max + inc;
  }
}

void IncDec16(uint16_t *val, uint16_t min, uint16_t max, int16_t inc) {
  if (inc > 0) {
    *val = *val == max - inc ? min : *val + inc;
  } else {
    *val = *val > min ? *val + inc : max + inc;
  }
}

void IncDecI16(int16_t *val, int16_t min, int16_t max, int16_t inc) {
  if (inc > 0) {
    *val = *val == max - inc ? min : *val + inc;
  } else {
    *val = *val > min ? *val + inc : max + inc;
  }
}

void IncDecI32(int32_t *val, int32_t min, int32_t max, int32_t inc) {
  if (inc > 0) {
    *val = *val == max - inc ? min : *val + inc;
  } else {
    *val = *val > min ? *val + inc : max + inc;
  }
}

void IncDec32(uint32_t *val, uint32_t min, uint32_t max, int32_t inc) {
  if (inc > 0) {
    *val = *val == max - inc ? min : *val + inc;
  } else {
    *val = *val > min ? *val + inc : max + inc;
  }
}

bool IsReadable(char *name) { return name[0] >= 32 && name[0] < 127; }

SQL GetSql(uint8_t level) {
  SQL sq = {0, 0, 255, 255, 255, 255};
  if (level == 0) {
    return sq;
  }

  sq.ro = ConvertDomain(level, 0, 10, 10, 180);
  sq.no = ConvertDomain(level, 0, 10, 64, 12);
  sq.go = ConvertDomain(level, 0, 10, 32, 6);

  sq.rc = sq.ro - 4;
  sq.nc = sq.gc = sq.no + 4;
  return sq;
}

uint32_t DeltaF(uint32_t f1, uint32_t f2) {
  return f1 > f2 ? f1 - f2 : f2 - f1;
}
