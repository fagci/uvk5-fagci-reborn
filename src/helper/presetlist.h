#ifndef PRESETLIST_HELPER_H
#define PRESETLIST_HELPER_H

#include "../radio.h"
#include <stdint.h>

#define PRESETS_SIZE_MAX 32

bool PRESETS_Load();
uint8_t PRESETS_Size();
Preset *PRESETS_Item(uint8_t i);
void PRESETS_SelectPresetRelative(bool next);
uint8_t PRESET_GetCurrentIndex();
uint8_t PRESET_Select(uint8_t i);
Preset *PRESET_ByFrequency(uint32_t f);
int8_t PRESET_SelectByFrequency(uint32_t f);
void PRESETS_SavePreset(uint8_t num, Preset *p);
void PRESETS_LoadPreset(uint8_t num, Preset *p);
void PRESETS_SaveCurrent();
bool PRESET_InRange(const uint32_t f, const Preset *p);
bool PRESET_InRangeOffset(const uint32_t f, const Preset *p);

uint16_t PRESETS_GetStepSize(Preset *p);
uint16_t PRESETS_GetSteps(Preset *p);
uint32_t PRESETS_GetF(Preset *p, uint16_t channel);
uint16_t PRESETS_GetChannel(Preset *p, uint32_t f);

extern Preset defaultPreset;
extern Preset *gCurrentPreset;

#endif /* end of include guard: PRESETLIST_HELPER_H */
