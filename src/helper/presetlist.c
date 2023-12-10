#include "presetlist.h"
#include "../settings.h"

Preset *gCurrentPreset;
static Preset presets[BANDS_COUNT] = {0};
static uint8_t loadedCount = 0;

bool PRESETS_Load() {
  if (loadedCount < BANDS_COUNT) {
    RADIO_LoadPreset(loadedCount, &presets[loadedCount]);
    loadedCount++;
    return false;
  }
  return true;
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

int8_t PRESET_GetCurrentIndex() {
  for (uint8_t i = 0; i < BANDS_COUNT; ++i) {
    if (gCurrentPreset == &presets[i]) {
      return i;
    }
  }
  return -1;
}
