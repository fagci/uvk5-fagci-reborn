#include "svc_listening.h"
#include "radio.h"

void SVC_LISTEN_Init(void) {}
void SVC_LISTEN_Update(void) { RADIO_UpdateMeasurements(); }
void SVC_LISTEN_Deinit(void) {}
