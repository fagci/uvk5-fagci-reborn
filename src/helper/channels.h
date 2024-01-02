#ifndef CHANNELS_H
#define CHANNELS_H

#include "../settings.h"
#include <stdint.h>

uint16_t CHANNELS_GetCountMax();
void CHANNELS_Load(uint16_t num, CH *p);
void CHANNELS_Save(uint16_t num, CH *p);
CH *CHANNELS_Get(uint16_t i);
bool CHANNELS_LoadBuf();
int16_t CHANNELS_Next(int16_t base, bool next);
void CHANNELS_Delete(uint16_t i);
bool CHANNELS_Existing(uint16_t i);

#endif /* end of include guard: CHANNELS_H */
