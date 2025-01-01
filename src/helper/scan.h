#ifndef SCAN_H
#define SCAN_H

#include <stdbool.h>
#include <stdint.h>

void SCAN_Start();
void SCAN_StartAB();
void SCAN_Stop();
void SCAN_ToggleDirection(bool up);
bool SCAN_IsFast();
uint32_t SCAN_GetTimeout();

#endif /* end of include guard: SCAN_H */
