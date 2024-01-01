#ifndef VFOS_H
#define VFOS_H

#include "../settings.h"
#include <stdint.h>

void VFOS_Load(uint16_t num, VFO *p);
void VFOS_Save(uint16_t num, VFO *p);

extern const uint32_t VFOS_OFFSET;

#endif /* end of include guard: VFOS_H */
