#include "systick.h"
#include "../external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"
#include "misc.h"
#include "system.h"

static const uint32_t TICK_MULTIPLIER = CPU_CLOCK_HZ / 1000000;

void SYSTICK_Init(void) { SysTick_Config(CPU_CLOCK_HZ / 1000); }

void SYSTICK_DelayTicks(const uint32_t ticks) {
  uint32_t n0 = SysTick->VAL;
  uint32_t np = n0;
  int32_t nc;

  do {
    nc = SysTick->VAL;
    if (nc >= np)
      n0 += SysTick->LOAD + 1;
    np = nc;
  } while (n0 - nc < ticks);
}

void SYSTICK_DelayUs(const uint32_t Delay) {
  SYSTICK_DelayTicks(Delay * TICK_MULTIPLIER);
}

void SYSTICK_Delay250ns(const uint32_t Delay) {
  SYSTICK_DelayTicks(Delay * TICK_MULTIPLIER / 4);
}
