#include "radio.h"
#include "driver/audio.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/system.h"
#include "driver/uart.h"
#include "external/printf/printf.h"
#include "helper/presetlist.h"
#include "inc/dp32g030/gpio.h"
#include "scheduler.h"
#include "settings.h"
#include <string.h>

VFO *gCurrentVFO;
VFO gVFO[2] = {0};

bool gIsListening = false;

const uint16_t StepFrequencyTable[14] = {
    1,   10,  50,  100,

    250, 500, 625, 833, 1000, 1250, 2500, 10000, 12500, 20000,
};

const uint32_t upConverterValues[3] = {0, 5000000, 12500000};
const char *upConverterFreqNames[3] = {"None", "50M", "125M"};

const char *modulationTypeOptions[5] = {"FM", "AM", "SSB", "BYP", "RAW"};
const char *vfoStateNames[] = {
    "NORMAL", "BUSY", "BAT LOW", "DISABLE", "TIMEOUT", "ALARM", "VOL HIGH",
};
const char *powerNames[] = {"LOW", "MID", "HIGH"};
const char *bwNames[3] = {"25k", "12.5k", "6.25k"};

const SquelchType sqTypeValues[4] = {
    SQUELCH_RSSI_NOISE_GLITCH,
    SQUELCH_RSSI_GLITCH,
    SQUELCH_RSSI_NOISE,
    SQUELCH_RSSI,
};
const char *sqTypeNames[4] = {"RNG", "RG", "RN", "R"};
const char *deviationNames[] = {"", "+", "-"};

void RADIO_SetupRegisters() {
  uint32_t Frequency = 0;

  GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, false);
  BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);
  BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);

  BK4819_SetFilterBandwidth(BK4819_FILTER_BW_WIDE);

  BK4819_SetupPowerAmplifier(0, 0);

  while (BK4819_ReadRegister(BK4819_REG_0C) & 1U) {
    BK4819_WriteRegister(BK4819_REG_02, 0);
    SYSTEM_DelayMs(1);
  }
  BK4819_WriteRegister(BK4819_REG_3F, 0);
  BK4819_WriteRegister(BK4819_REG_7D, 0xE94F); // TODO: maybe add some value
  BK4819_SetFrequency(Frequency);
  BK4819_SelectFilter(Frequency);
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, true);
  BK4819_WriteRegister(BK4819_REG_48, 0xB3A8);

  BK4819_DisableScramble();

  BK4819_DisableVox();
  BK4819_DisableDTMF();

  BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_SQUELCH_FOUND |
                                          BK4819_REG_3F_SQUELCH_LOST);
  BK4819_WriteRegister(0x40, (BK4819_ReadRegister(0x40) & ~(0b11111111111)) |
                                 0b10110101010);
}

uint32_t GetScreenF(uint32_t f) {
  return f - upConverterValues[gSettings.upconverter];
}
uint32_t GetTuneF(uint32_t f) {
  return f + upConverterValues[gSettings.upconverter];
}

void RADIO_ToggleRX(bool on) {
  if (gIsListening == on) {
    return;
  }

  gIsListening = on;

  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, on);

  if (on) {
    BK4819_ToggleAFDAC(true);
    BK4819_ToggleAFBit(true);
    SYSTEM_DelayMs(10);
    AUDIO_ToggleSpeaker(true);
  } else {
    AUDIO_ToggleSpeaker(false);
    SYSTEM_DelayMs(10);
    BK4819_ToggleAFDAC(false);
    BK4819_ToggleAFBit(false);
  }
}

static void onVfoUpdate() {
  TaskRemove(RADIO_SaveCurrentVFO);
  TaskAdd("VFO save", RADIO_SaveCurrentVFO, 5000, false);
}

static void onPresetUpdate() {
  TaskRemove(PRESETS_SaveCurrent);
  TaskAdd("Preset save", PRESETS_SaveCurrent, 5000, false);
}

void RADIO_ToggleModulation() {
  if (gCurrentVFO->modulation == MOD_RAW) {
    gCurrentVFO->modulation = MOD_FM;
  } else {
    ++gCurrentVFO->modulation;
  }
  BK4819_SetModulation(gCurrentVFO->modulation);
  onVfoUpdate();
}

void RADIO_UpdateStep(bool inc) {
  if (inc && gCurrentPreset->band.step < STEP_100_0kHz) {
    ++gCurrentPreset->band.step;
  } else if (!inc && gCurrentPreset->band.step > 0) {
    --gCurrentPreset->band.step;
  } else {
    return;
  }
  onPresetUpdate();
}

void RADIO_ToggleListeningBW() {
  if (gCurrentVFO->bw == BK4819_FILTER_BW_NARROWER) {
    gCurrentVFO->bw = BK4819_FILTER_BW_WIDE;
  } else {
    ++gCurrentVFO->bw;
  }

  BK4819_SetFilterBandwidth(gCurrentVFO->bw);
  onVfoUpdate();
}

void RADIO_TuneTo(uint32_t f) {
  memset(gCurrentVFO->name, 0, sizeof(gCurrentVFO->name));
  gCurrentVFO->fRX = f;
  PRESET_SelectByFrequency(gCurrentVFO->fRX);
  BK4819_TuneTo(f);
  onVfoUpdate();
}

void RADIO_TuneToSave(uint32_t f) {
  UART_logf(1, "TUNETOSAVE: %u", f);
  sprintf(gCurrentVFO->name, "");
  gCurrentVFO->fRX = f;
  PRESET_SelectByFrequency(gCurrentVFO->fRX);
  BK4819_TuneTo(f);
  RADIO_SaveCurrentVFO();
}

void RADIO_SaveCurrentVFO() {
  RADIO_SaveChannel(gSettings.activeChannel, gCurrentVFO);
}

void RADIO_LoadCurrentVFO() {
  RADIO_LoadChannel(0, &gVFO[0]);
  RADIO_LoadChannel(1, &gVFO[1]);
  gCurrentVFO = &gVFO[gSettings.activeChannel];
  PRESET_SelectByFrequency(gCurrentVFO->fRX);
}

void RADIO_SetSquelch(uint8_t sq) {
  BK4819_Squelch(gCurrentPreset->band.squelch = sq, gCurrentVFO->fRX);
  onPresetUpdate();
}

void RADIO_SetSquelchType(SquelchType t) {
  BK4819_SquelchType(gCurrentPreset->band.squelchType = t);
  onPresetUpdate();
}

void RADIO_SetGain(uint8_t gainIndex) {
  BK4819_SetGain(gCurrentPreset->band.gainIndex = gainIndex);
  onPresetUpdate();
}

void RADIO_SetupByCurrentVFO() {
  PRESET_SelectByFrequency(gCurrentVFO->fRX);
  BK4819_SquelchType(gCurrentPreset->band.squelchType);
  BK4819_Squelch(gCurrentPreset->band.squelch, gCurrentVFO->fRX);
  BK4819_TuneTo(gCurrentVFO->fRX);
  BK4819_SetFilterBandwidth(gCurrentVFO->bw);
  BK4819_SetModulation(gCurrentVFO->modulation);
  BK4819_SetGain(gCurrentPreset->band.gainIndex);
}

void RADIO_SetupBandParams(Band *b) {
  BK4819_SelectFilter(b->bounds.start);
  BK4819_SquelchType(b->squelchType);
  BK4819_Squelch(b->squelch, b->bounds.start);
  BK4819_SetFilterBandwidth(b->bw);
  BK4819_SetModulation(b->modulation);
  BK4819_SetGain(b->gainIndex);
}

void RADIO_LoadUserChannel(uint16_t num, VFO *p) {
  RADIO_LoadChannel(num + 2, p);
}

void RADIO_SaveUserChannel(uint16_t num, VFO *p) {
  RADIO_SaveChannel(num + 2, p);
}

void RADIO_LoadChannel(uint16_t num, VFO *p) {
  EEPROM_ReadBuffer(CHANNELS_OFFSET - (num + 1) * VFO_SIZE, p, VFO_SIZE);
}

void RADIO_SaveChannel(uint16_t num, VFO *p) {
  EEPROM_WriteBuffer(CHANNELS_OFFSET - (num + 1) * VFO_SIZE, p, VFO_SIZE);
}
