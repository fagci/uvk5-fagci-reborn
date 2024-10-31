#ifndef CHANNELS_H
#define CHANNELS_H

#define SCANLIST_MAX 1024

#include "../settings.h"
#include <stdint.h>

uint16_t CHANNELS_GetCountMax();
void CHANNELS_Load(int16_t num, CH *p);
void CHANNELS_Save(int16_t num, CH *p);
CH *CHANNELS_Get(int16_t i);
bool CHANNELS_LoadBuf();
int16_t CHANNELS_Next(int16_t base, bool next);
void CHANNELS_Delete(int16_t i);
bool CHANNELS_Existing(int16_t i);
uint8_t CHANNELS_Scanlists(int16_t i);
void CHANNELS_LoadScanlist(uint8_t n);
F CHANNELS_GetRX(int16_t num);

extern int16_t gScanlistSize;
extern uint16_t gScanlist[SCANLIST_MAX];

#endif /* end of include guard: CHANNELS_H */
