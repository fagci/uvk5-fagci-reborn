#include "radio.h"
#include "driver/audio.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/system.h"
#include "inc/dp32g030/gpio.h"
#include "scheduler.h"

VFO gCurrentVfo;
UpconverterTypes gUpconverterType = UPCONVERTER_OFF;
bool gIsListening = false;

const uint16_t StepFrequencyTable[12] = {
    1,   10,  50,  100,

    250, 500, 625, 833, 1000, 1250, 2500, 10000,
};

const uint32_t upConverterValues[3] = {0, 5000000, 12500000};
const char *upConverterFreqNames[3] = {"None", "50M", "125M"};

const char *modulationTypeOptions[5] = {" FM", " AM", "SSB", "BYP", "RAW"};
const char *vfoStateNames[] = {
    "NORMAL", "BUSY", "BAT LOW", "DISABLE", "TIMEOUT", "ALARM", "VOL HIGH",
};
const char *powerNames[] = {"LOW", "MID", "HIGH"};
const char *bwNames[3] = {"  25k", "12.5k", "6.25k"};
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
  return f - upConverterValues[gUpconverterType];
}
uint32_t GetTuneF(uint32_t f) {
  return f + upConverterValues[gUpconverterType];
}

void RADIO_ToggleRX(bool on) {
  if (gIsListening == on) {
    return;
  }

  gIsListening = on;

  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, on);

  AUDIO_ToggleSpeaker(on);
  BK4819_ToggleAFDAC(on);
  BK4819_ToggleAFBit(on);
}

static void onVfoUpdate() {
  TaskRemove(RADIO_SaveCurrentVFO);
  TaskAdd("VFO save", RADIO_SaveCurrentVFO, 5000, false);
}

void RADIO_ToggleModulation() {
  if (gCurrentVfo.modulation == MOD_RAW) {
    gCurrentVfo.modulation = MOD_FM;
  } else {
    ++gCurrentVfo.modulation;
  }
  BK4819_SetModulation(gCurrentVfo.modulation);
  onVfoUpdate();
}

void RADIO_UpdateStep(bool inc) {
  if (inc && gCurrentVfo.step < STEP_100_0kHz) {
    ++gCurrentVfo.step;
  } else if (!inc && gCurrentVfo.step > 0) {
    --gCurrentVfo.step;
  } else {
    return;
  }
  onVfoUpdate();
}

void RADIO_ToggleListeningBW() {
  if (gCurrentVfo.bw == BK4819_FILTER_BW_NARROWER) {
    gCurrentVfo.bw = BK4819_FILTER_BW_WIDE;
  } else {
    ++gCurrentVfo.bw;
  }

  BK4819_SetFilterBandwidth(gCurrentVfo.bw);
  onVfoUpdate();
}

void RADIO_TuneTo(uint32_t f, bool precise) {
  gCurrentVfo.fRX = f;
  BK4819_TuneTo(f, precise);
  onVfoUpdate();
}

void RADIO_SaveCurrentVFO() {
  EEPROM_WriteBuffer(CURRENT_VFO_OFFSET, &gCurrentVfo, VFO_SIZE);
}

void RADIO_LoadCurrentVFO() {
  EEPROM_ReadBuffer(CURRENT_VFO_OFFSET, &gCurrentVfo, VFO_SIZE);
}
