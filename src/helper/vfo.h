#ifndef VFO_H
#define VFO_H

#include "channels.h"
#include <stdint.h>

void VFO_Load(uint16_t num, CH *p);
void VFO_Save(uint16_t num, CH *p);

#endif /* end of include guard: VFO_H */
