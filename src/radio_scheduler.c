#include "radio_scheduler.h"
#include "radio.h"
#include "scheduler.h"
#include <stdbool.h>
#include <stdint.h>

static uint32_t ctxInactiveTimeout = 0;
static uint32_t ctxActiveTimeout = 0;
static bool lastContextState = false;
static bool newContext = true;

static uint32_t ctxInactiveTimeoutTime = 5000;
static uint32_t ctxActiveTimeoutTime = 1000;

static void setTimeout(uint32_t *v, uint32_t t) {
  *v = elapsedMilliseconds + t;
}
static bool checkTimeout(uint32_t *v) { return *v >= elapsedMilliseconds; }

static bool powersave(void) {
  // check powersave timings and return state
  return false; // FIXME: not implmented
}

static bool executeContext(void) {
  // execute current context and return active state
  RADIO_UpdateMeasurements();
  Loot *msm = &gLoot[gSettings.activeVFO];

  bool ctxState = msm->open;
  if (lastContextState != ctxState || newContext) {
    newContext = false;
    lastContextState = ctxState;
    if (ctxState) {
      setTimeout(&ctxActiveTimeout, ctxActiveTimeoutTime);
    } else {
      setTimeout(&ctxInactiveTimeout, ctxInactiveTimeoutTime);
    }
  }

  return ctxState;
}

static void contextSwitch(void) {
  // switch context
  // and reset context timers?
  RADIO_NextFreq(true);
  newContext = true;
}

void RADIO_SCHEDULER_Update(void) {
  if (powersave()) {
    return;
  }

  if (executeContext()) {
    if (checkTimeout(&ctxActiveTimeout)) {
      contextSwitch();
      return;
    }
  } else {
    if (checkTimeout(&ctxInactiveTimeout)) {
      contextSwitch();
      return;
    }
  }
}
