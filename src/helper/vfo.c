#include "vfo.h"
#include "channels.h"

CH vfos[10] = {0};
uint8_t vfosCount = 0;

void VFO_LoadVFOS() {
  vfosCount = 0;
  for (int16_t i = 0; i < CHANNELS_GetCountMax(); ++i) {
    if (CHANNELS_GetType(i) == CH_VFO) {
      CHANNELS_Load(i, &vfos[vfosCount]);
      vfosCount++;
    }
  }
}

uint8_t VFO_GetVfosCount() { return vfosCount; }
CH *VFO_GetVfo(uint8_t i) { return &vfos[i]; }
