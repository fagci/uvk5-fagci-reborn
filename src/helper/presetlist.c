#include "presetlist.h"
#include "../driver/eeprom.h"
#include "../driver/uart.h"
#include "../helper/measurements.h"
#include "../settings.h"

Preset *gCurrentPreset;
static Preset presets[PRESETS_SIZE_MAX] = {0};
static uint8_t loadedCount = 0;

// to use instead of predefined when we need to keep step, etc
static Preset defaultPreset = {.band = (Band){.name = "default"}};

void PRESETS_SavePreset(uint8_t num, Preset *p) {
  EEPROM_WriteBuffer(BANDS_OFFSET + num * PRESET_SIZE, p, PRESET_SIZE);
}

void PRESETS_LoadPreset(uint8_t num, Preset *p) {
  EEPROM_ReadBuffer(BANDS_OFFSET + num * PRESET_SIZE, p, PRESET_SIZE);
}

void PRESETS_SaveCurrent() {
  if (gCurrentPreset != &defaultPreset) {
    PRESETS_SavePreset(gSettings.activePreset, gCurrentPreset);
  }
}

uint8_t PRESETS_Size() { return gSettings.presetsCount; }

Preset *PRESETS_Item(uint8_t i) { return &presets[i]; }

void PRESETS_SelectPresetRelative(bool next) {
  uint8_t activePreset = gSettings.activePreset;
  IncDec8(&activePreset, 0, gSettings.presetsCount, next ? 1 : -1);
  gSettings.activePreset = activePreset;
  gCurrentPreset = &presets[gSettings.activePreset];
  gCurrentVFO->fRX = gCurrentPreset->band.bounds.start;
  SETTINGS_DelayedSave();
}

int8_t PRESET_GetCurrentIndex() { return gSettings.activePreset; }

int8_t PRESET_Select(int8_t i) {
  gCurrentPreset = &presets[i];
  gSettings.activePreset = i;
  return i;
}

int8_t PRESET_SelectByFrequency(uint32_t f) {
  UART_logf(1, "Choosing from %u presets", gSettings.presetsCount);
  for (uint8_t i = 0; i < gSettings.presetsCount; ++i) {
    FRange *range = &presets[i].band.bounds;
    UART_logf(1, "%u <= %u <= %u", range->start, f, range->end);
    if (f >= range->start && f <= range->end) {
      return PRESET_Select(i);
    }
  }
  gCurrentPreset = &defaultPreset; // TODO: make preset between near bands
  return -1;
}

bool PRESETS_Load() {
  if (loadedCount < gSettings.presetsCount) {
    PRESETS_LoadPreset(loadedCount, &presets[loadedCount]);
    loadedCount++;
    return false;
  }
  return true;
}
