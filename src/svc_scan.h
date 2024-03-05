#ifndef SVC_SCAN_H
#define SVC_SCAN_H

#include <stdint.h>

void SVC_SCAN_Init();
void SVC_SCAN_Update();
void SVC_SCAN_Deinit();

extern bool gScanForward;
extern bool gScanRedraw;
extern uint16_t gScanSwitchT;
extern void (*gScanFn)(bool);
extern char *SCAN_TIMEOUT_NAMES[11];
extern uint32_t SCAN_TIMEOUTS[11];

#endif /* end of include guard: SVC_SCAN_H */
