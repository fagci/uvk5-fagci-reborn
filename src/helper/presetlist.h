#ifndef PRESETLIST_HELPER_H
#define PRESETLIST_HELPER_H

#include "../radio.h"
#include <stdint.h>

bool PRESETS_Load();
uint8_t PRESETS_Size();
Preset *PRESETS_Item(uint8_t i);
void PRESETS_SelectPresetRelative(bool next);
int8_t PRESET_GetCurrentIndex();

extern Preset *gCurrentPreset;

#endif /* end of include guard: PRESETLIST_HELPER_H */
