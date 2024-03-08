#ifndef FREQUENCY_H
#define FREQUENCY_H

#include <stdint.h>

typedef struct {
  uint32_t start : 27;
  uint32_t end : 27;
} __attribute__((packed)) FRange;

uint32_t GetScreenF(uint32_t f);
uint32_t GetTuneF(uint32_t f);

extern const FRange STOCK_BANDS[12];

#endif /* end of include guard: FREQUENCY_H */
