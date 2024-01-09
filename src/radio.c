#include "radio.h"
#include "driver/audio.h"
#include "driver/backlight.h"
#include "driver/bk1080.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/system.h"
#include "driver/uart.h"
#include "external/printf/printf.h"
#include "helper/adapter.h"
#include "helper/channels.h"
#include "helper/lootlist.h"
#include "helper/measurements.h"
#include "helper/presetlist.h"
#include "helper/vfos.h"
#include "inc/dp32g030/gpio.h"
#include "scheduler.h"
#include "settings.h"
#include <string.h>

VFO *gCurrentVFO;
VFO gVFO[2] = {0};

Loot *gCurrentLoot;
Loot gLoot[2] = {0};

char gVFONames[2][10] = {0};

bool gIsListening = false;
bool gMonitorMode = false;

bool isBK1080 = false;

const uint16_t StepFrequencyTable[14] = {
    1,   10,  50,  100,

    250, 500, 625, 833, 1000, 1250, 2500, 10000, 12500, 20000,
};

const uint32_t upConverterValues[3] = {0, 5000000, 12500000};
const char *upConverterFreqNames[3] = {"None", "50M", "125M"};

const char *modulationTypeOptions[6] = {"FM", "AM", "SSB", "BYP", "RAW", "WFM"};
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

  BK4819_WriteRegister(BK4819_REG_3F, 0);
  /* BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_SQUELCH_FOUND |
                                          BK4819_REG_3F_SQUELCH_LOST); */
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
    if (gSettings.backlightOnSquelch != BL_SQL_OFF) {
      BACKLIGHT_On();
    }
  } else {
    AUDIO_ToggleSpeaker(false);
    SYSTEM_DelayMs(10);
    BK4819_ToggleAFDAC(false);
    BK4819_ToggleAFBit(false);
    if (gSettings.backlightOnSquelch == BL_SQL_OPEN) {
      BACKLIGHT_Toggle(false);
    }
  }
}

static void onVfoUpdate() {
  TaskRemove(RADIO_SaveCurrentVFO);
  TaskAdd("VFO save", RADIO_SaveCurrentVFO, 2000, false);
}

static void onPresetUpdate() {
  TaskRemove(PRESETS_SaveCurrent);
  TaskAdd("Preset save", PRESETS_SaveCurrent, 2000, false);
}

bool RADIO_IsBK1080Range(uint32_t f) { return f >= 6400000 && f <= 10800000; }

void RADIO_ToggleBK1080(bool on) {
  if (on == isBK1080) {
    return;
  }
  isBK1080 = on;

  AUDIO_ToggleSpeaker(false);
  SYSTEM_DelayMs(10);
  if (isBK1080) {
    BK4819_Idle();
    BK1080_Init(gCurrentVFO->fRX, true);
  } else {
    BK1080_Init(0, false);
    BK4819_RX_TurnOn();
  }
  AUDIO_ToggleSpeaker(true);
  RADIO_SetupByCurrentVFO();
}

void RADIO_SetModulationByPreset() {
  ModulationType mod = gCurrentPreset->band.modulation;
  if (mod == MOD_WFM) {
    if (RADIO_IsBK1080Range(gCurrentVFO->fRX)) {
      RADIO_ToggleBK1080(true);
      return;
    }
    gCurrentPreset->band.modulation = MOD_FM;
  }
  RADIO_ToggleBK1080(false);
  BK4819_SetModulation(gCurrentPreset->band.modulation);
  onPresetUpdate();
}

void RADIO_ToggleModulation() {
  if (gCurrentPreset->band.modulation == MOD_WFM) {
    gCurrentPreset->band.modulation = MOD_FM;
  } else {
    ++gCurrentPreset->band.modulation;
  }
  RADIO_SetModulationByPreset();
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
  if (gCurrentPreset->band.bw == BK4819_FILTER_BW_NARROWER) {
    gCurrentPreset->band.bw = BK4819_FILTER_BW_WIDE;
  } else {
    ++gCurrentPreset->band.bw;
  }

  BK4819_SetFilterBandwidth(gCurrentPreset->band.bw);
  onPresetUpdate();
}

void RADIO_TuneTo(uint32_t f) {
  gCurrentVFO->isMrMode = false;
  gCurrentVFO->fRX = f;
  LOOT_Replace(&gLoot[gSettings.activeVFO], f);
  RADIO_SetupByCurrentVFO();
  onVfoUpdate();
}

void RADIO_TuneToSave(uint32_t f) {
  gCurrentVFO->isMrMode = false;
  gCurrentVFO->fRX = f;
  LOOT_Replace(&gLoot[gSettings.activeVFO], f);
  RADIO_SetupByCurrentVFO();
  RADIO_SaveCurrentVFO();
}

void RADIO_SaveCurrentVFO() { VFOS_Save(gSettings.activeVFO, gCurrentVFO); }

void RADIO_VfoLoadCH(uint8_t i) {
  CH ch;
  CHANNELS_Load(gVFO[i].channel, &ch);
  CH2VFO(&ch, &gVFO[i]);
  strncpy(gVFONames[i], ch.name, 9);
  gVFO[i].isMrMode = true;
}

void RADIO_LoadCurrentVFO() {
  for (uint8_t i = 0; i < 2; ++i) {
    VFOS_Load(i, &gVFO[i]);
    if (gVFO[i].isMrMode) {
      RADIO_VfoLoadCH(i);
    }
    LOOT_Replace(&gLoot[i], gVFO[i].fRX);
  }

  gCurrentVFO = &gVFO[gSettings.activeVFO];
  gCurrentLoot = &gLoot[gSettings.activeVFO];
  PRESET_SelectByFrequency(gCurrentVFO->fRX);
}

void RADIO_SetSquelch(uint8_t sq) {
  gCurrentPreset->band.squelch = sq;
  BK4819_Squelch(sq, gCurrentPreset->band.bounds.start);

  // RADIO_SetupByCurrentVFO(); TODO: set AF to previous instead of just RX ON
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

void RADIO_SetupBandParams(Band *b) {
  BK4819_SelectFilter(b->bounds.start);
  BK4819_SquelchType(b->squelchType);
  BK4819_Squelch(b->squelch, b->bounds.start);
  BK4819_SetFilterBandwidth(b->bw);
  BK4819_SetModulation(b->modulation);
  BK4819_SetGain(b->gainIndex);
}

void RADIO_SetupByCurrentVFO() {
  PRESET_SelectByFrequency(gCurrentVFO->fRX);

  gCurrentVFO->modulation = gCurrentPreset->band.modulation;
  gCurrentVFO->bw = gCurrentPreset->band.bw;

  RADIO_ToggleBK1080(gCurrentVFO->modulation == MOD_WFM &&
                     RADIO_IsBK1080Range(gCurrentVFO->fRX));

  if (isBK1080) {
    BK1080_SetFrequency(gCurrentVFO->fRX);
  } else {
    RADIO_SetupBandParams(&gCurrentPreset->band);
    BK4819_TuneTo(gCurrentVFO->fRX);
  }
}

typedef struct {
  uint16_t ro;
  uint16_t rc;
  uint8_t no;
  uint8_t nc;
  uint8_t go;
  uint8_t gc;
} SQLParams;

static SQLParams sq = {255, 255, 0, 0, 0, 0};

Loot gMeasurements = {0};

static bool isSquelchOpen() {
  bool open = gMeasurements.rssi > sq.ro && gMeasurements.noise < sq.no &&
              gMeasurements.glitch < sq.go;

  if (gMeasurements.rssi < sq.rc || gMeasurements.noise > sq.nc ||
      gMeasurements.glitch > sq.gc) {
    open = false;
  }
  return open;
}

void RADIO_UpdateMeasurements() {
  gMeasurements.rssi = BK4819_GetRSSI();
  gMeasurements.noise = BK4819_GetNoise();
  gMeasurements.glitch = BK4819_GetGlitch();

  gMeasurements.open = isSquelchOpen();
}

void RADIO_TuneToPure(uint32_t f) {
  Preset *preset = PRESET_ByFrequency(f);
  LOOT_Replace(&gMeasurements, f);

  RADIO_ToggleBK1080(preset->band.modulation == MOD_WFM &&
                     RADIO_IsBK1080Range(f));

  if (isBK1080) {
    BK1080_SetFrequency(f);
  } else {
    RADIO_SetupBandParams(&preset->band);
    BK4819_TuneTo(f);
  }

  uint8_t sql = preset->band.squelch;
  uint8_t band = f > VHF_UHF_BOUND ? 1 : 0;
  sq.ro = SQ[band][0][sql];
  sq.rc = SQ[band][1][sql];
  sq.no = SQ[band][2][sql];
  sq.nc = SQ[band][3][sql];
  sq.go = SQ[band][4][sql];
  sq.gc = SQ[band][5][sql];
}

void RADIO_EnableToneDetection() {
  BK4819_SetCTCSSFrequency(670);
  BK4819_SetTailDetection(550);
  BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_CxCSS_TAIL/*  |
                                          BK4819_REG_3F_SQUELCH_LOST |
                                          BK4819_REG_3F_SQUELCH_FOUND */);
}

void RADIO_NextCH(bool next) {
  int16_t i;
  if (gCurrentVFO->isMrMode) {
    i = CHANNELS_Next(gCurrentVFO->channel, next);
    if (i > -1) {
      gCurrentVFO->channel = i;
      RADIO_VfoLoadCH(gSettings.activeVFO);
    }
  } else {
    CH ch;
    i = gCurrentVFO->channel;

    if (!CHANNELS_Existing(gCurrentVFO->channel)) {
      i = CHANNELS_Next(gCurrentVFO->channel, true);
      if (i == -1) {
        return;
      }
    }

    gCurrentVFO->channel = i;
    CHANNELS_Load(gCurrentVFO->channel, &ch);
    CH2VFO(&ch, gCurrentVFO);
    strncpy(gVFONames[gSettings.activeVFO], ch.name, 9);
    gCurrentVFO->isMrMode = true;
  }
  onVfoUpdate();
  RADIO_SetupByCurrentVFO();
}

void RADIO_NextVFO(bool next) {
  gSettings.activeVFO = !gSettings.activeVFO;
  gCurrentVFO = &gVFO[gSettings.activeVFO];
  gCurrentLoot = &gLoot[gSettings.activeVFO];
  RADIO_SetupByCurrentVFO();
  SETTINGS_Save();
}

void RADIO_ToggleVfoMR() {
  if (gCurrentVFO->isMrMode) {
    gCurrentVFO->isMrMode = false;
  } else {
    RADIO_NextCH(true);
  }
  RADIO_SaveCurrentVFO();
}

void RADIO_UpdateSquelchLevel(bool next) {
  uint8_t sq = gCurrentPreset->band.squelch;
  if (!next && sq > 0) {
    sq--;
  }
  if (next && sq < 9) {
    sq++;
  }
  RADIO_SetSquelch(sq);
}

// TODO: бесшовное
void RADIO_NextFreq(bool next) {
  int8_t dir = next ? 1 : -1;

  if (gCurrentVFO->isMrMode) {
    RADIO_NextCH(next);
    return;
  }

  Preset *nextPreset = PRESET_ByFrequency(gCurrentVFO->fRX + dir);
  if (nextPreset != gCurrentPreset && nextPreset != &defaultPreset) {
    if (next) {
      RADIO_TuneTo(nextPreset->band.bounds.start);
    } else {
      RADIO_TuneTo(nextPreset->band.bounds.end -
                   nextPreset->band.bounds.end %
                       StepFrequencyTable[nextPreset->band.step]);
    }
  } else {
    RADIO_TuneTo(gCurrentVFO->fRX +
                 StepFrequencyTable[nextPreset->band.step] * dir);
  }
}
