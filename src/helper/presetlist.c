#include "presetlist.h"
#include "../settings.h"

Preset *gCurrentPreset;
static Preset presets[32] = {0};
static uint8_t loadedCount = 0;

// to use instead of predefined when we need to keep step, etc
static Preset defaultPreset = {0};

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
  gCurrentPreset = &presets[gSettings.activePreset];
  gCurrentVFO->fRX = gCurrentPreset->band.bounds.start;
  SETTINGS_DelayedSave();
}

int8_t PRESET_GetCurrentIndex() { return gSettings.activePreset; }

int8_t PRESET_SelectByFrequency(uint32_t f) {
  for (uint8_t i = 0; i < PRESETS_Size(); ++i) {
    Preset *p = PRESETS_Item(i);
    if (f >= p->band.bounds.start && f <= p->band.bounds.end) {
      gCurrentPreset = p;
      gSettings.activePreset = i;
      return i;
    }
  }
  gCurrentPreset = &defaultPreset; // TODO: make preset between near bands
  return -1;
}
