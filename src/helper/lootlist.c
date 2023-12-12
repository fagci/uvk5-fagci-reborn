#include "lootlist.h"
#include "../scheduler.h"

#define LOOT_SIZE_MAX 16

static Loot loot[LOOT_SIZE_MAX] = {0};
static int8_t lootIndex = -1;

Loot *LOOT_Get(uint32_t f) {
  for (uint8_t i = 0; i < LOOT_Size(); ++i) {
    if ((&loot[i])->vfo->fRX == f) {
      return &loot[i];
    }
  }
  return NULL;
}

Loot *LOOT_Add(uint32_t f) {
  Loot *p = LOOT_Get(f);
  if (p) {
    return p;
  }
  if (LOOT_Size() < LOOT_SIZE_MAX) {
    lootIndex++;
    loot[lootIndex] = (Loot){
        .vfo = &(VFO){f},
        .firstTime = elapsedMilliseconds,
        .lastTimeCheck = elapsedMilliseconds,
        .lastTimeOpen = elapsedMilliseconds,
        .duration = 0,
        .rssi = 0,
        .noise = 65535,
        .open = true, // as we add it when open
    };
    return &loot[lootIndex];
  }
  return NULL;
}

Loot *LOOT_AddVFO(VFO *vfo) {
  Loot *p = LOOT_Get(vfo->fRX);
  if (p) {
    return p;
  }
  p = LOOT_Add(vfo->fRX);
  p->vfo = vfo;
  return p;
}

void LOOT_Clear() { lootIndex = -1; }

uint8_t LOOT_Size() { return lootIndex + 1; }

void LOOT_Standby() {
  for (uint8_t i = 0; i < LOOT_Size(); ++i) {
    Loot *p = &loot[i];
    p->open = false;
    p->lastTimeCheck = elapsedMilliseconds;
  }
}

static void swap(Loot *a, Loot *b) {
  Loot tmp = *a;
  *a = *b;
  *b = tmp;
}

bool LOOT_SortByLastOpenTime(Loot *a, Loot *b) {
  return a->lastTimeOpen < b->lastTimeOpen;
}

static void Sort(Loot *items, uint16_t n, bool (*compare)(Loot *a, Loot *b)) {
  for (uint16_t i = 0; i < n - 1; i++) {
    bool swapped = false;
    for (uint16_t j = 0; j < n - i - 1; j++) {
      if (compare(&items[j], &items[j + 1])) {
        swap(&items[j], &items[j + 1]);
        swapped = true;
      }
    }
    if (!swapped) {
      break;
    }
  }
}

void LOOT_Sort(bool (*compare)(Loot *a, Loot *b)) {
  Sort(loot, LOOT_Size(), compare);
}

Loot *LOOT_Item(uint8_t i) { return &loot[i]; }
