#ifndef LOOTLIST_HELPER_H

#define LOOTLIST_HELPER_H

#include "channels.h"
#include <stdbool.h>
#include <stdint.h>

#define LOOT_SIZE_MAX 320

typedef struct {
  uint32_t f;
  uint32_t lastTimeOpen;
  uint16_t duration;
  uint8_t snr;
  uint8_t cd;
  uint8_t ct;
  bool open : 1;
  bool blacklist : 1;
  bool whitelist : 1;
} Loot;

typedef struct {
  uint32_t f;
  uint32_t lastTimeOpen;
  uint16_t duration;
  uint16_t rssi;
  uint8_t noise;
  uint8_t glitch;
  uint8_t snr;
  uint8_t cd;
  uint8_t ct;
  bool open : 1;
  bool blacklist : 1;
  bool whitelist : 1;
} Measurement;

int16_t LOOT_IndexOf(Loot *loot);
void LOOT_BlacklistLast();
void LOOT_WhitelistLast();
Loot *LOOT_Get(uint32_t f);
Loot *LOOT_AddEx(uint32_t f, bool reuse);
Loot *LOOT_Add(uint32_t f);
void LOOT_Remove(uint16_t i);
void LOOT_Clear();
void LOOT_Standby();
uint16_t LOOT_Size();
Loot *LOOT_Item(uint16_t i);
void LOOT_UpdateEx(Loot *loot, Measurement *msm);
void LOOT_Update(Measurement *msm);
void LOOT_Replace(Measurement *loot, uint32_t f);

void LOOT_Sort(bool (*compare)(const Loot *a, const Loot *b), bool reverse);

bool LOOT_SortByLastOpenTime(const Loot *a, const Loot *b);
bool LOOT_SortByDuration(const Loot *a, const Loot *b);
bool LOOT_SortByF(const Loot *a, const Loot *b);
bool LOOT_SortByBlacklist(const Loot *a, const Loot *b);

void LOOT_RemoveBlacklisted(void);
CH LOOT_ToCh(const Loot *loot);

extern Loot *gLastActiveLoot;
extern int16_t gLastActiveLootIndex;
extern const char *CH_TYPE_NAMES[6];

#endif
