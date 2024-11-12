#include "reset.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../helper/vfos.h"
#include "../settings.h"
#include "../ui/graphics.h"
#include "ARMCM0.h"

static uint32_t bytesMax = 0;
static uint32_t bytesWrote = 0;
static uint16_t channelsMax = 0;
static uint16_t channelsWrote = 0;
static uint8_t presetsWrote = 0;
static uint8_t vfosWrote = 0;
static bool settingsWrote = 0;

static EEPROMType eepromType;

static VFO defaultVFOs[2] = {
    (VFO){
        .rx.f = 14550000,
        .channel = -1,
        .modulation = MOD_PRST,
        .radio = RADIO_UNKNOWN,
    },
    (VFO){
        .rx.f = 43307500,
        .channel = -1,
        .modulation = MOD_PRST,
        .radio = RADIO_UNKNOWN,
    },
};

static void startReset(EEPROMType t) {
  eepromType = t;
  gSettings.eepromType = eepromType;
  presetsWrote = 0;
  vfosWrote = 0;
  bytesWrote = 0;
  channelsWrote = 0;
  settingsWrote = false;
  channelsMax = CHANNELS_GetCountMax();
  bytesMax = ARRAY_SIZE(defaultPresets) * PRESET_SIZE + channelsMax * CH_SIZE;
}

void RESET_Init(void) {
  gSettings.keylock = false;
  eepromType = EEPROM_A;
}

void RESET_Update(void) {
  if (eepromType < EEPROM_BL24C64) {
    return;
  }
  if (!settingsWrote) {
    gSettings = defaultSettings;
    gSettings.eepromType = eepromType;
    settingsWrote = true;
    bytesWrote += SETTINGS_SIZE;
  } else if (vfosWrote < ARRAY_SIZE(defaultVFOs)) {
    VFOS_Save(vfosWrote, &defaultVFOs[vfosWrote]);
    vfosWrote++;
    bytesWrote += VFO_SIZE;
  } else if (presetsWrote < ARRAY_SIZE(defaultPresets)) {
    Preset *p = &defaultPresets[presetsWrote];
    p->band.gainIndex = 18;
    p->band.squelch = 3;
    p->band.squelchType = SQUELCH_RSSI_NOISE_GLITCH;
    if (p->band.txF < 3000000) {
      p->radio = RADIO_SI4732; // TODO: if SI existing
                               // default is RADIO_BK4819
    }
    PRESETS_SavePreset(presetsWrote, p);
    presetsWrote++;
    bytesWrote += PRESET_SIZE;
  } else if (channelsWrote < channelsMax) {
    CHANNELS_Delete(channelsWrote);
    channelsWrote++;
    bytesWrote += CH_SIZE;
  } else {
    SETTINGS_Save();
    NVIC_SystemReset();
  }
  gRedrawScreen = true;
}

void RESET_Render(void) {
  uint8_t progressX = ConvertDomain(bytesWrote, 0, bytesMax, 1, LCD_WIDTH - 2);
  uint8_t POS_Y = LCD_HEIGHT / 2;

  if (eepromType < EEPROM_BL24C64) {
    for (uint8_t t = EEPROM_BL24C64; t <= EEPROM_M24M02; t++) {
      uint8_t i = t - EEPROM_BL24C64;
      PrintMedium(2, 18 + i * 8, "%u: %s", i + 1, EEPROM_TYPE_NAMES[t]);
    }
    return;
  }

  DrawRect(0, POS_Y, LCD_WIDTH, 10, C_FILL);
  FillRect(1, POS_Y, progressX, 10, C_FILL);
  PrintMedium(0, 16, "%u/%u", channelsWrote, channelsMax);
  PrintMedium(0, 24, "%lu", bytesMax);
  PrintMediumEx(LCD_XCENTER, POS_Y + 8, POS_C, C_INVERT, "%u%",
                bytesWrote * 100 / bytesMax);
}

bool RESET_key(KEY_Code_t k, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld && k > KEY_0) {
    uint8_t t = EEPROM_BL24C64 + k - 1;
    if (t < ARRAY_SIZE(EEPROM_TYPE_NAMES)) {
      startReset(t);
      return true;
    }
  }
  return false;
}
