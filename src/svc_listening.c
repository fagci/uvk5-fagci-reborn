#include "svc_listening.h"
#include "radio.h"
#include <stddef.h>

void (*gListenFn)(void) = NULL;

void SVC_LISTEN_Init(void) {
  if (!gListenFn) {
    gListenFn = RADIO_UpdateMeasurements;
  }
}

void SVC_LISTEN_Update(void) { gListenFn(); }

void SVC_LISTEN_Deinit(void) { gListenFn = NULL; }
