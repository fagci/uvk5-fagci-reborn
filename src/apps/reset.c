#include "reset.h"
#include "../driver/st7565.h"
#include "../external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "../settings.h"
#include "../ui/graphics.h"

static uint16_t channelsMax = 0;
static uint16_t channelsWrote = 0;
static uint8_t presetsWrote = 0;
static uint8_t vfosWrote = 0;
static bool settingsWrote = 0;

static uint8_t eepromType = 255;

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
  eepromType = UINT8_MAX;
}

void RESET_Update(void) {
  gRedrawScreen = true;
  if (eepromType == UINT8_MAX) {
    return;
  }

  if (!settingsWrote) {
    gSettings = defaultSettings;
    gSettings.eepromType = eepromType;
    settingsWrote = true;
    return;
  }

  if (vfosWrote < ARRAY_SIZE(gVFO)) {
    VFO *vfo = &gVFO[vfosWrote];
    vfo->channel = -1;
    vfo->modulation = MOD_FM;
    vfo->bw = BK4819_FILTER_BW_12k;
    vfo->radio = RADIO_BK4819;
    vfo->txF = 0;
    vfo->offsetDir = OFFSET_NONE;
    vfo->allowTx = false;
    vfo->gainIndex = AUTO_GAIN_INDEX;
    vfo->code.rx.type = 0;
    vfo->code.tx.type = 0;
    vfo->meta.readonly = false;
    vfo->misc.lastUsedFreq = vfo->rxF;
    vfo->meta.type = TYPE_VFO;
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
    p->meta.type = TYPE_PRESET;
    p->radio = RADIO_BK4819;
    if (hasSi) {
      if (p->txF < 3000000) {
        p->radio = RADIO_SI4732;
      }
    } else if (p->txF < 1500000) {
      // skip not supported presets
      presetsWrote++;
      return;
    }

    if (p->rxF >= 6400000 && p->txF <= 10800000) {
      p->radio = hasSi ? RADIO_SI4732 : RADIO_BK1080;
    }
    CHANNELS_Save(channelsMax - 2 - presetsWrote - 1, p);
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

  if (eepromType == UINT8_MAX) {
    for (uint8_t i = 0; i < ARRAY_SIZE(EEPROM_TYPE_NAMES); ++i) {
      PrintMedium(2, 18 + i * 8, "%u: %s", i + 1, EEPROM_TYPE_NAMES[i]);
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
    uint8_t t = k - 1;
    if (t < ARRAY_SIZE(EEPROM_TYPE_NAMES)) {
      startReset(t);
      return true;
    }
  }
  return false;
}
