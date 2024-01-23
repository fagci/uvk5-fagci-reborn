#include "presetlist.h"
#include "../driver/eeprom.h"
#include "../driver/uart.h"
#include "../helper/measurements.h"
#include "../settings.h"

Preset *gCurrentPreset;
static Preset presets[PRESETS_SIZE_MAX] = {0};
static uint8_t loadedCount = 0;

// to use instead of predefined when we need to keep step, etc
Preset defaultPreset = {
    .band =
        (Band){
            .name = "default",
            .step = STEP_25_0kHz,
            .bw = BK4819_FILTER_BW_WIDE,
            .gainIndex = 90,
            .squelch = 3,
            .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
        },
};

void PRESETS_SavePreset(uint8_t num, Preset *p) {
  EEPROM_WriteBuffer(PRESETS_OFFSET + num * PRESET_SIZE, p, PRESET_SIZE);
}

void PRESETS_LoadPreset(uint8_t num, Preset *p) {
  EEPROM_ReadBuffer(PRESETS_OFFSET + num * PRESET_SIZE, p, PRESET_SIZE);
}

void PRESETS_SaveCurrent(void) {
  if (gCurrentPreset != &defaultPreset) {
    PRESETS_SavePreset(gSettings.activePreset, gCurrentPreset);
  }
}

uint8_t PRESETS_Size(void) { return gSettings.presetsCount; }

Preset *PRESETS_Item(uint8_t i) { return &presets[i]; }

void PRESETS_SelectPresetRelative(bool next) {
  uint8_t activePreset = gSettings.activePreset;
  IncDec8(&activePreset, 0, PRESETS_Size(), next ? 1 : -1);
  gSettings.activePreset = activePreset;
  gCurrentPreset = &presets[gSettings.activePreset];
  gCurrentVFO->fRX = gCurrentPreset->band.bounds.start;
  SETTINGS_DelayedSave();
}

uint8_t PRESET_GetCurrentIndex(void) { return gSettings.activePreset; }

uint8_t PRESET_Select(uint8_t i) {
  gCurrentPreset = &presets[i];
  gSettings.activePreset = i;
  return i;
}

bool PRESET_InRange(const uint32_t f, const Preset *p) {
  return f >= p->band.bounds.start && f <= p->band.bounds.end;
}

bool PRESET_InRangeOffset(const uint32_t f, const Preset *p) {
  return f >= p->band.bounds.start + p->offset && f <= p->band.bounds.end + p->offset;
}

int8_t PRESET_SelectByFrequency(uint32_t f) {
  if (PRESET_InRange(f, gCurrentPreset)) {
    return gSettings.activePreset;
  }
  for (uint8_t i = 0; i < PRESETS_Size(); ++i) {
    if (PRESET_InRange(f, &presets[i])) {
      return PRESET_Select(i);
    }
  }
  gCurrentPreset = &defaultPreset; // TODO: make preset between near bands
  return -1;
}

Preset *PRESET_ByFrequency(uint32_t f) {
  for (uint8_t i = 0; i < PRESETS_Size(); ++i) {
    FRange *range = &presets[i].band.bounds;
    if (f >= range->start && f <= range->end) {
      return &presets[i];
    }
  }
  return &defaultPreset;
}

bool PRESETS_Load(void) {
  if (loadedCount < PRESETS_Size()) {
    PRESETS_LoadPreset(loadedCount, &presets[loadedCount]);
    loadedCount++;
    return false;
  }
  return true;
}

uint16_t PRESETS_GetStepSize(Preset *p) {
  return StepFrequencyTable[p->band.step];
}

uint16_t PRESETS_GetSteps(Preset *p) {
  return (p->band.bounds.end - p->band.bounds.start) / PRESETS_GetStepSize(p) +
         1;
}

uint32_t PRESETS_GetF(Preset *p, uint16_t channel) {
  return p->band.bounds.start + channel * PRESETS_GetStepSize(p);
}

uint16_t PRESETS_GetChannel(Preset *p, uint32_t f) {
  return (f - p->band.bounds.start) / PRESETS_GetStepSize(p);
}
