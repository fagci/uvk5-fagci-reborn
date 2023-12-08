#include "presetlist.h"
#include "../settings.h"

static Preset presets[BANDS_COUNT] = {0};

void PRESETS_Load() {
  for (uint8_t i = 0; i < BANDS_COUNT; ++i) {
    RADIO_LoadPreset(i, &presets[i]);
  }
}

uint8_t PRESETS_Size() { return BANDS_COUNT; }
Preset *PRESETS_Item(uint8_t i) { return &presets[i]; }
void PRESETS_SelectPresetRelative(bool next) {
  if (next) {
    if (gSettings.activePreset < BANDS_COUNT - 1) {
      gSettings.activePreset++;
    } else {
      gSettings.activePreset = 0;
    }
  } else {
    if (gSettings.activePreset > 0) {
      gSettings.activePreset--;
    } else {
      gSettings.activePreset = BANDS_COUNT - 1;
    }
  }
  SETTINGS_DelayedSave();
}
