#include "scheduler.h"

uint32_t Now(void) { return pdTICKS_TO_MS(xTaskGetTickCount()); }

void SetTimeout(uint32_t *v, uint32_t t) {
  *v = t == UINT32_MAX ? UINT32_MAX : Now() + t;
}

bool CheckTimeout(uint32_t *v) { return Now() >= *v; }
