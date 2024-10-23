#ifndef PRESETLIST_HELPER_H
#define PRESETLIST_HELPER_H

#include "../radio.h"
#include <stdint.h>

#define PRESETS_COUNT 40

bool PRESETS_Load();
int8_t PRESETS_Size();
Preset *PRESETS_Item(int8_t i);
int8_t PRESET_IndexOf(Preset *p);
void PRESETS_SelectPresetRelative(bool next);
int8_t PRESET_GetCurrentIndex();
void PRESET_Select(int8_t i);
Preset *PRESET_ByFrequency(uint32_t f);
int8_t PRESET_SelectByFrequency(uint32_t f);
void PRESETS_SavePreset(int8_t num, Preset *p);
void PRESETS_LoadPreset(int8_t num, Preset *p);
void PRESETS_SaveCurrent();
bool PRESET_InRange(const uint32_t f, const Preset *p);
bool PRESET_InRangeOffset(const uint32_t f, const Preset *p);

uint16_t PRESETS_GetStepSize(Preset *p);
uint32_t PRESETS_GetSteps(Preset *p);
uint32_t PRESETS_GetF(Preset *p, uint32_t channel);
uint32_t PRESETS_GetChannel(Preset *p, uint32_t f);

extern Preset defaultPresets[PRESETS_COUNT];
extern Preset defaultPreset;
extern Preset *gCurrentPreset;

#endif /* end of include guard: PRESETLIST_HELPER_H */
