#ifndef SVC_SCAN_H
#define SVC_SCAN_H

#include <stdbool.h>
#include <stdint.h>

void SVC_SCAN_Init(void);
void SVC_SCAN_Update(void);
void SVC_SCAN_Deinit(void);

extern bool gScanForward;
extern uint16_t gScanSwitchT;
extern void (*gScanFn)(bool);

#endif /* end of include guard: SVC_SCAN_H */
