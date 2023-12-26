#ifndef CHANNELS_H
#define CHANNELS_H

#include "../settings.h"
#include <stdint.h>

uint16_t CHANNELS_GetCountMax();
void CHANNELS_Load(uint16_t num, VFO *p);
void CHANNELS_Save(uint16_t num, VFO *p);
void CHANNELS_LoadUser(uint16_t num, VFO *p);
void CHANNELS_SaveUser(uint16_t num, VFO *p);
VFO *CHANNELS_Get(uint16_t i);
bool CHANNELS_LoadBuf();

#endif /* end of include guard: CHANNELS_H */
