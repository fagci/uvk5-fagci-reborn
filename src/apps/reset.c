#include "reset.h"
#include "../driver/st7565.h"
#include "../external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../settings.h"
#include "../ui/graphics.h"

static uint16_t channelsMax = 0;
static uint16_t channelsWrote = 0;
static uint8_t presetsWrote = 0;
static uint8_t vfosWrote = 0;
static bool settingsWrote = 0;

static EEPROMType eepromType;

static VFO defaultVFOs[2] = {
    (VFO){
        .name = "VFO 1",
        .rxF = 14550000,
    },
    (VFO){
        .name = "VFO 2",
        .rxF = 43307500,
    },
};

static void startReset(EEPROMType t) {
  eepromType = t;
  gSettings.eepromType = eepromType;

  settingsWrote = false;
  vfosWrote = 0;
  presetsWrote = 0;
  channelsWrote = 0;

  channelsMax = CHANNELS_GetCountMax();
}

void RESET_Init(void) {
  gSettings.keylock = false;
  eepromType = EEPROM_A;
}

void RESET_Update(void) {
  gRedrawScreen = true;
  if (eepromType < EEPROM_BL24C64) {
    return;
  }

  if (!settingsWrote) {
    gSettings = defaultSettings;
    gSettings.eepromType = eepromType;
    settingsWrote = true;
    return;
  }

  if (vfosWrote < ARRAY_SIZE(defaultVFOs)) {
    VFO *vfo = &defaultVFOs[vfosWrote];
    vfo->channel = -1;
    vfo->modulation = MOD_FM;
    vfo->radio = RADIO_BK4819;
    vfo->txF = 0;
    vfo->offsetDir = OFFSET_NONE;
    vfo->allowTx = false;
    vfo->gainIndex = AUTO_GAIN_INDEX;
    vfo->code.rx.type = 0;
    vfo->code.tx.type = 0;
    vfo->readonly = false;
    vfo->misc.lastUsedFreq = vfo->rxF;
    vfo->type = TYPE_VFO;
    vfo->squelch.value = 4;
    vfo->step = STEP_25_0kHz;
    CHANNELS_Save(channelsMax - 2 + vfosWrote, vfo);
    vfosWrote++;
    return;
  }

  if (presetsWrote < ARRAY_SIZE(defaultPresets)) {
    Preset *p = &defaultPresets[presetsWrote];
    p->gainIndex = 18;
    p->squelch.value = 4;
    p->squelch.type = SQUELCH_RSSI_NOISE_GLITCH;
    p->type = TYPE_PRESET;
    if (p->txF < 3000000) {
      p->radio = RADIO_SI4732; // TODO: if SI existing
                               // default is RADIO_BK4819
    }
    CHANNELS_Save(channelsMax - 2 - ARRAY_SIZE(defaultPresets) + presetsWrote,
                  p);
    presetsWrote++;
    return;
  }

  if (channelsWrote < channelsMax - 2 - ARRAY_SIZE(defaultPresets)) {
    CHANNELS_Delete(channelsWrote);
    channelsWrote++;
    return;
  }
  SETTINGS_Save();
  NVIC_SystemReset();
}

void RESET_Render(void) {
  uint8_t POS_Y = LCD_HEIGHT / 2;

  if (eepromType < EEPROM_BL24C64) {
    for (uint8_t t = EEPROM_BL24C64; t <= EEPROM_M24M02; t++) {
      uint8_t i = t - EEPROM_BL24C64;
      PrintMedium(2, 18 + i * 8, "%u: %s", i + 1, EEPROM_TYPE_NAMES[t]);
    }
    return;
  }

  uint8_t progressX =
      ConvertDomain(channelsWrote, 0, channelsMax, 1, LCD_WIDTH - 2);
  DrawRect(0, POS_Y, LCD_WIDTH, 10, C_FILL);
  FillRect(1, POS_Y, progressX, 10, C_FILL);
  PrintMedium(0, 16, "%u/%u", channelsWrote, channelsMax);
  PrintMediumEx(LCD_XCENTER, POS_Y + 8, POS_C, C_INVERT, "%u%",
                channelsWrote * 100 / channelsMax);
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
