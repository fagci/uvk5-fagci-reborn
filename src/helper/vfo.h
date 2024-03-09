#ifndef VFO_H
#define VFO_H

#include "../globals.h"
#include <stdint.h>

void VFO_LoadVFOS();
uint8_t VFO_GetVfosCount();
CH *VFO_GetVfo(uint8_t i);

#endif /* end of include guard: VFO_H */
