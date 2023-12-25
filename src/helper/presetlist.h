#ifndef PRESETLIST_HELPER_H
#define PRESETLIST_HELPER_H

#include "../radio.h"
#include <stdint.h>

#define PRESETS_SIZE_MAX 32

bool PRESETS_Load();
uint8_t PRESETS_Size();
Preset *PRESETS_Item(uint8_t i);
void PRESETS_SelectPresetRelative(bool next);
int8_t PRESET_GetCurrentIndex();
int8_t PRESET_SelectByFrequency(uint32_t f);
void PRESETS_SavePreset(uint8_t num, Preset *p);
void PRESETS_LoadPreset(uint8_t num, Preset *p);
void PRESETS_SaveCurrent();

extern Preset *gCurrentPreset;

#endif /* end of include guard: PRESETLIST_HELPER_H */
