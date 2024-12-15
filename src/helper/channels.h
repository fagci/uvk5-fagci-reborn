#ifndef CHANNELS_H
#define CHANNELS_H

#define SCANLIST_MAX 1024
#define PRESETS_COUNT_MAX 60

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
uint32_t CHANNELS_GetSteps(Preset *p);
uint32_t CHANNELS_GetF(Preset *p, uint32_t channel);
uint32_t CHANNELS_GetChannel(Preset *p, uint32_t f);
CHMeta CHANNELS_GetMeta(int16_t num);

bool PRESETS_Load();
int8_t PRESETS_Size();
Preset PRESETS_Item(int8_t i);
int8_t PRESET_IndexOf(Preset p);
void PRESETS_SelectPresetRelative(bool next);
void PRESETS_SelectPresetRelativeByScanlist(bool next);
void PRESET_Select(int8_t i);
Preset PRESET_ByFrequency(uint32_t f);
int8_t PRESET_SelectByFrequency(uint32_t f);
void PRESETS_SaveCurrent();
bool PRESET_InRange(const uint32_t f, const Preset p);
bool CHANNELS_IsScanlistable(CHType type);
bool CHANNELS_IsFreqable(CHType type);
uint16_t CHANNELS_ScanlistByKey(uint16_t sl, KEY_Code_t key, bool longPress);

extern Preset defaultPresets[PRESETS_COUNT_MAX];
extern Preset defaultPreset;
extern Preset gCurrentPreset;

extern int16_t gScanlistSize;
extern uint16_t gScanlist[SCANLIST_MAX];
extern uint16_t gPresetlist[SCANLIST_MAX];

#endif /* end of include guard: CHANNELS_H */
