#include "lootlist.h"

static void swap(Peak *a, Peak *b) {
  Peak tmp = *a;
  *a = *b;
  *b = tmp;
}

bool SortByLastOpenTime(Peak *a, Peak *b) {
  return a->lastTimeOpen < b->lastTimeOpen;
}

void Sort(Peak *items, uint16_t n, bool (*compare)(Peak *a, Peak *b)) {
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
