#include "lootlist.h"
#include "../driver/uart.h"
#include "../scheduler.h"

static Loot loot[LOOT_SIZE_MAX] = {0};
static int8_t lootIndex = -1;

Loot *gLastActiveLoot = NULL;

void LOOT_BlacklistLast() {
  if (gLastActiveLoot) {
    gLastActiveLoot->blacklist = true;
  }
}

void LOOT_GoodKnownLast() {
  if (gLastActiveLoot) {
    gLastActiveLoot->goodKnown = true;
  }
}

Loot *LOOT_Get(uint32_t f) {
  for (uint8_t i = 0; i < LOOT_Size(); ++i) {
    if ((&loot[i])->f == f) {
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
        .f = f,
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

void LOOT_Remove(uint8_t i) {
  for (uint8_t _i = i; _i < LOOT_Size() - 1; ++_i) {
    loot[_i] = loot[_i + 1];
  }
  lootIndex--;
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

bool LOOT_SortByDuration(Loot *a, Loot *b) { return a->duration > b->duration; }

bool LOOT_SortByF(Loot *a, Loot *b) { return a->f > b->f; }

bool LOOT_SortByBlacklist(Loot *a, Loot *b) {
  return a->blacklist > b->blacklist;
}

static void Sort(Loot *items, uint16_t n, bool (*compare)(Loot *a, Loot *b),
                 bool reverse) {
  for (uint16_t i = 0; i < n - 1; i++) {
    bool swapped = false;
    for (uint16_t j = 0; j < n - i - 1; j++) {
      if (compare(&items[j], &items[j + 1]) ^ reverse) {
        swap(&items[j], &items[j + 1]);
        swapped = true;
      }
    }
    if (!swapped) {
      break;
    }
  }
}

void LOOT_Sort(bool (*compare)(Loot *a, Loot *b), bool reverse) {
  Sort(loot, LOOT_Size(), compare, reverse);
}

Loot *LOOT_Item(uint8_t i) { return &loot[i]; }

void LOOT_ReplaceItem(uint8_t i, uint32_t f) {
  Loot *item = LOOT_Item(i);
  item->f = f;
  item->firstTime = elapsedMilliseconds;
  item->lastTimeCheck = elapsedMilliseconds;
  item->lastTimeOpen = elapsedMilliseconds;
  item->duration = 0;
  item->rssi = 0;
  item->noise = 65535;
}

void LOOT_Update(Loot *msm) {
  Loot *peak = LOOT_Get(msm->f);

  if (peak == NULL && msm->open) {
    peak = LOOT_Add(msm->f);
    UART_logf(1, "[LOOT] %u", msm->f);
  }

  if (peak == NULL) {
    return;
  }

  if (peak->blacklist || peak->goodKnown) {
    msm->open = false;
  }

  peak->noise = msm->noise;
  peak->rssi = msm->rssi;

  if (peak->open) {
    peak->duration += elapsedMilliseconds - peak->lastTimeCheck;
    gLastActiveLoot = peak;
  }
  if (msm->open) {
    uint32_t cd = 0;
    uint16_t ct = 0;
    BK4819_CssScanResult_t res = BK4819_GetCxCSSScanResult(&cd, &ct);
    switch (res) {
    case BK4819_CSS_RESULT_CDCSS:
      peak->cd = cd;
      break;
    case BK4819_CSS_RESULT_CTCSS:
      peak->ct = ct;
      break;
    default:
      break;
    }
    peak->lastTimeOpen = elapsedMilliseconds;
  }
  peak->lastTimeCheck = elapsedMilliseconds;
  peak->open = msm->open;

  if (msm->blacklist) {
    peak->blacklist = true;
  }
}
