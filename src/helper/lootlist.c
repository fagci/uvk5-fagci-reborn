#include "lootlist.h"
#include "../dcs.h"
#include "../driver/bk4819.h"
#include "../external/printf/printf.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../svc.h"
#include "bands.h"
#include <stdint.h>

static Loot loot[LOOT_SIZE_MAX] = {0};
static int16_t lootIndex = -1;
static uint32_t lastTimeCheck = 0;

Loot *gLastActiveLoot = NULL;
int16_t gLastActiveLootIndex = -1;

void LOOT_BlacklistLast(void) {
  if (gLastActiveLoot) {
    gLastActiveLoot->whitelist = false;
    gLastActiveLoot->blacklist = true;
  }
}

void LOOT_WhitelistLast(void) {
  if (gLastActiveLoot) {
    gLastActiveLoot->blacklist = false;
    gLastActiveLoot->whitelist = true;
  }
}

Loot *LOOT_Get(uint32_t f) {
  for (uint16_t i = 0; i < LOOT_Size(); ++i) {
    if ((&loot[i])->f == f) {
      return &loot[i];
    }
  }
  return NULL;
}

int16_t LOOT_IndexOf(Loot *item) {
  for (uint16_t i = 0; i < LOOT_Size(); ++i) {
    if (&item[i] == item) {
      return i;
    }
  }
  return -1;
}

Loot *LOOT_AddEx(uint32_t f, bool reuse) {
  if (reuse) {
    Loot *p = LOOT_Get(f);
    if (p) {
      return p;
    }
  }
  if (LOOT_Size() < LOOT_SIZE_MAX) {
    lootIndex++;
  }
  lastTimeCheck = Now();
  loot[lootIndex] = (Loot){
      .f = f,
      .lastTimeOpen = Now(),
      .duration = 0,
      .rssi = 0,
      .noise = UINT8_MAX,
      .cd = 0xFF,
      .ct = 0xFF,
      .open = true, // as we add it when open
  };
  return &loot[lootIndex];
}

Loot *LOOT_Add(uint32_t f) { return LOOT_AddEx(f, true); }

void LOOT_Remove(uint16_t i) {
  if (LOOT_Size()) {
    for (; i < LOOT_Size() - 1; ++i) {
      loot[i] = loot[i + 1];
    }
    lootIndex--;
  }
}

void LOOT_Clear(void) { lootIndex = -1; }

uint16_t LOOT_Size(void) { return lootIndex + 1; }

void LOOT_Standby(void) {
  for (uint16_t i = 0; i < LOOT_Size(); ++i) {
    Loot *p = &loot[i];
    p->open = false;
  }
  lastTimeCheck = Now();
}

static void swap(Loot *a, Loot *b) {
  Loot tmp = *a;
  *a = *b;
  *b = tmp;
}

bool LOOT_SortByLastOpenTime(const Loot *a, const Loot *b) {
  return a->lastTimeOpen < b->lastTimeOpen;
}

bool LOOT_SortByDuration(const Loot *a, const Loot *b) {
  return a->duration > b->duration;
}

bool LOOT_SortByF(const Loot *a, const Loot *b) { return a->f > b->f; }

bool LOOT_SortByBlacklist(const Loot *a, const Loot *b) {
  return a->blacklist > b->blacklist;
}

static void Sort(Loot *items, uint16_t n,
                 bool (*compare)(const Loot *a, const Loot *b), bool reverse) {
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

void LOOT_Sort(bool (*compare)(const Loot *a, const Loot *b), bool reverse) {
  Sort(loot, LOOT_Size(), compare, reverse);
}

Loot *LOOT_Item(uint16_t i) { return &loot[i]; }

void LOOT_Replace(Loot *item, uint32_t f) {
  item->f = f;
  item->open = false;
  item->lastTimeOpen = 0;
  item->duration = 0;
  item->rssi = 0;
  item->noise = UINT8_MAX;
  item->ct = 0xFF;
  item->cd = 0xFF;
  lastTimeCheck = Now();
}

void LOOT_ReplaceItem(uint16_t i, uint32_t f) { LOOT_Replace(LOOT_Item(i), f); }

void LOOT_UpdateEx(Loot *item, Loot *msm) {
  if (item == NULL) {
    return;
  }

  if (SVC_Running(SVC_SCAN) &&
      (item->blacklist || (item->whitelist && !gMonitorMode))) {
    msm->open = false;
  }

  item->rssi = msm->rssi;

  if (item->open) {
    item->duration += Now() - lastTimeCheck;
    gLastActiveLoot = item;
    gLastActiveLootIndex = LOOT_IndexOf(item);
  }
  if (msm->open) {
    uint32_t cd = 0;
    uint16_t ct = 0;
    uint8_t Code = 0;
    BK4819_CssScanResult_t res = BK4819_GetCxCSSScanResult(&cd, &ct);
    switch (res) {
    case BK4819_CSS_RESULT_CDCSS:
      Code = DCS_GetCdcssCode(cd);
      if (Code != 0xFF) {
        item->cd = Code;
      }
      break;
    case BK4819_CSS_RESULT_CTCSS:
      Code = DCS_GetCtcssCode(ct);
      if (Code != 0xFF) {
        item->ct = Code;
      }
      break;
    default:
      break;
    }
    item->lastTimeOpen = Now();
  }
  lastTimeCheck = Now();
  item->open = msm->open;
  msm->ct = item->ct;
  msm->cd = item->cd;

  if (msm->blacklist) {
    item->blacklist = true;
  }
}

void LOOT_Update(Loot *msm) {
  Loot *item = LOOT_Get(msm->f);

  if (item == NULL && msm->open) {
    item = LOOT_Add(msm->f);
  }

  LOOT_UpdateEx(item, msm);
}

void LOOT_RemoveBlacklisted(void) {
  LOOT_Sort(LOOT_SortByBlacklist, true);
  for (uint16_t i = 0; i < LOOT_Size(); ++i) {
    if (loot[i].blacklist) {
      lootIndex = i;
      return;
    }
  }
}

CH LOOT_ToCh(const Loot *loot) {
  // TODO: automatic params by simple "band plan"
  Band p = BANDS_ByFrequency(loot->f);
  CH ch = {
      .meta.type = TYPE_CH,
      .rxF = loot->f,
      .txF = 0,
      .code =
          (CodeRXTX){
              .rx.type = CODE_TYPE_OFF,
              .tx.type = CODE_TYPE_OFF,
              .rx.value = 0,
              .tx.value = 0,
          },
      .radio = p.radio,
      .modulation = p.modulation,
      .power = p.power,
      .bw = p.bw,
  };

  snprintf(ch.name, 9, "%u.%05u", ch.rxF / MHZ, ch.rxF % MHZ);

  if (loot->ct != 255) {
    ch.code.tx.type = CODE_TYPE_CONTINUOUS_TONE;
    ch.code.tx.value = loot->ct;
  } else if (loot->cd != 255) {
    ch.code.tx.type = CODE_TYPE_DIGITAL;
    ch.code.tx.value = loot->cd;
  }

  return ch;
}
