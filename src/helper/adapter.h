#ifndef ADAPTER_H
#define ADAPTER_H

#include "channels.h"
#include "vfos.h"

void VFO2CH(VFO *src, CH *dst);
void CH2VFO(CH *src, VFO *dst);

#endif /* end of include guard: ADAPTER_H */
