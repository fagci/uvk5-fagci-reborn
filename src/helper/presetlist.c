#include "presetlist.h"
#include "../settings.h"

Preset *gCurrentPreset;
static Preset presets[32] = {0};
static uint8_t loadedCount = 0;

bool PRESETS_Load() {
  if (loadedCount < gSettings.presetsCount) {
    RADIO_LoadPreset(loadedCount, &presets[loadedCount]);
    loadedCount++;
    return false;
  }
  return true;
}

uint8_t PRESETS_Size() { return gSettings.presetsCount; }
Preset *PRESETS_Item(uint8_t i) { return &presets[i]; }
void PRESETS_SelectPresetRelative(bool next) {
  if (next) {
    if (gSettings.activePreset < gSettings.presetsCount - 1) {
      gSettings.activePreset++;
    } else {
      gSettings.activePreset = 0;
    }
  } else {
    if (gSettings.activePreset > 0) {
      gSettings.activePreset--;
    } else {
      gSettings.activePreset = gSettings.presetsCount - 1;
    }
  }
  gCurrentVFO->fRX = presets[gSettings.activePreset].band.bounds.start;
  SETTINGS_DelayedSave();
}

int8_t PRESET_GetCurrentIndex() {
  for (uint8_t i = 0; i < gSettings.presetsCount; ++i) {
    if (gCurrentPreset == &presets[i]) {
      return i;
    }
  }
  return -1;
}
