#ifndef LOOTLIST_HELPER_H

#define LOOTLIST_HELPER_H

#include <stdbool.h>
#include <stdint.h>

#define LOOT_SIZE_MAX 128

typedef struct {
  uint32_t f;
  uint32_t firstTime;
  uint32_t lastTimeCheck;
  uint32_t lastTimeOpen;
  uint16_t duration;
  uint16_t rssi;
  uint8_t noise;
  uint16_t cd; // 8 bit actually, but ROM mem usage gains
  uint16_t ct; // same
  bool open;
  bool blacklist;
  bool goodKnown;
} Loot;

int16_t LOOT_IndexOf(Loot *loot);
void LOOT_BlacklistLast();
void LOOT_GoodKnownLast();
Loot *LOOT_Get(uint32_t f);
Loot *LOOT_AddEx(uint32_t f, bool reuse);
Loot *LOOT_Add(uint32_t f);
void LOOT_Remove(uint8_t i);
void LOOT_Clear();
void LOOT_Standby();
uint8_t LOOT_Size();
Loot *LOOT_Item(uint8_t i);
void LOOT_UpdateEx(Loot *loot, Loot *msm);
void LOOT_Update(Loot *msm);
void LOOT_Replace(Loot *loot, uint32_t f);
void LOOT_ReplaceItem(uint8_t i, uint32_t f);

void LOOT_Sort(bool (*compare)(Loot *a, Loot *b), bool reverse);

bool LOOT_SortByLastOpenTime(Loot *a, Loot *b);
bool LOOT_SortByDuration(Loot *a, Loot *b);
bool LOOT_SortByF(Loot *a, Loot *b);
bool LOOT_SortByBlacklist(Loot *a, Loot *b);

void LOOT_RemoveBlacklisted(void);

extern Loot *gLastActiveLoot;
extern int16_t gLastActiveLootIndex;

#endif
