#ifndef CHANNELS_H
#define CHANNELS_H

#include "../globals.h"
#include <stdint.h>

int16_t CHANNELS_GetCountMax();
void CHANNELS_Load(int16_t num, CH *p);
void CHANNELS_Save(int16_t num, CH *p);
CH *CHANNELS_Get(int16_t i);
bool CHANNELS_LoadBuf();
int16_t CHANNELS_Next(int16_t base, bool next);
void CHANNELS_Delete(int16_t i);
bool CHANNELS_Existing(int16_t i);
ChannelType CHANNELS_GetType(int16_t i);
uint8_t CHANNELS_Scanlists(int16_t i);
void CHANNELS_LoadScanlist(uint8_t n);
void CHANNELS_LoadVFOS();

extern int16_t gScanlistSize;
extern int32_t gScanlist[350];

#endif /* end of include guard: CHANNELS_H */
