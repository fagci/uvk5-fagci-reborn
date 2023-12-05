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
  uint32_t cd;
  uint16_t ct;
  bool open;
} Loot;

bool LOOT_SortByLastOpenTime(Loot *a, Loot *b);
Loot *LOOT_Get(uint32_t f);
Loot *LOOT_Add(uint32_t f);
void LOOT_Clear();
void LOOT_Standby();
uint8_t LOOT_Size();
void LOOT_Sort(bool (*compare)(Loot *a, Loot *b));
Loot *LOOT_Item(uint8_t i);

#endif
