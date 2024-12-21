#ifndef CHANNELS_H
#define CHANNELS_H

#define SCANLIST_MAX 1024
#define BANDS_COUNT_MAX 60

#include "../settings.h"
#include <stdint.h>

uint16_t CHANNELS_GetCountMax();

void CHANNELS_Load(int16_t num, CH *p);
void CHANNELS_Save(int16_t num, CH *p);
bool CHANNELS_LoadBuf();
int16_t CHANNELS_Next(int16_t base, bool next);
void CHANNELS_Delete(int16_t i);
bool CHANNELS_Existing(int16_t i);
uint8_t CHANNELS_Scanlists(int16_t i);
void CHANNELS_LoadScanlist(CHType type, uint16_t n);
void CHANNELS_LoadBlacklistToLoot();

uint16_t CHANNELS_GetStepSize(CH *p);
uint32_t CHANNELS_GetSteps(Band *p);
uint32_t CHANNELS_GetF(Band *p, uint32_t channel);
uint32_t CHANNELS_GetChannel(Band *p, uint32_t f);
CHMeta CHANNELS_GetMeta(int16_t num);

bool BANDS_Load();
Band BANDS_Item(int8_t i);
int8_t BAND_IndexOf(Band p);
void BANDS_SelectBandRelative(bool next);
bool BANDS_SelectBandRelativeByScanlist(bool next);
void BAND_Select(int8_t i);
void BAND_SelectScan(int8_t i);
Band BAND_ByFrequency(uint32_t f);
bool BAND_SelectByFrequency(uint32_t f);
void BANDS_SaveCurrent();
bool BAND_InRange(const uint32_t f, const Band p);
bool CHANNELS_IsScanlistable(CHType type);
bool CHANNELS_IsFreqable(CHType type);
uint16_t CHANNELS_ScanlistByKey(uint16_t sl, KEY_Code_t key, bool longPress);

extern Band defaultBands[BANDS_COUNT_MAX];
extern Band defaultBand;
extern Band gCurrentBand;

extern int16_t gScanlistSize;
extern uint16_t gScanlist[SCANLIST_MAX];
extern uint16_t gBandlist[SCANLIST_MAX];

#endif /* end of include guard: CHANNELS_H */
