#ifndef LOOTLIST_HELPER_H

#define LOOTLIST_HELPER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint32_t f;
  uint32_t firstTime;
  uint32_t lastTimeCheck;
  uint32_t lastTimeOpen;
  uint32_t duration;
  uint16_t rssi;
  uint16_t noise;
  bool open;
} Peak;

bool SortByLastOpenTime(Peak *a, Peak *b);
void Sort(Peak *items, uint16_t n, bool (*compare)(Peak *a, Peak *b));

#endif
