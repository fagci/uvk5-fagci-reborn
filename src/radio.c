#include "radio.h"
#include "apps/apps.h"
#include "driver/audio.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "driver/gpio.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "helper/adapter.h"
#include "helper/battery.h"
#include "helper/channels.h"
#include "helper/lootlist.h"
#include "helper/measurements.h"
#include "helper/presetlist.h"
#include "helper/vfos.h"
#include "inc/dp32g030/gpio.h"
#include "scheduler.h"
#include "settings.h"
#include <string.h>

VFO *radio;
VFO gVFO[2] = {0};
Preset *gVFOPresets[2] = {0};

Loot *gCurrentLoot;
Loot gLoot[2] = {0};

typedef struct {
  uint16_t ro;
  uint16_t rc;
  uint8_t no;
  uint8_t nc;
  uint8_t go;
  uint8_t gc;
} SQLParams;

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
const char *powerNames[] = {"LOW", "MID", "HIGH"};
const char *bwNames[3] = {"25k", "12.5k", "6.25k"};
const char *TX_STATE_NAMES[7] = {"TX Off",    "TX On",    "VOL HIGH",
                                 "BAT LOW",   "DISABLED", "UPCONVERTER",
                                 "HIGH POWER"};

const SquelchType sqTypeValues[4] = {
    SQUELCH_RSSI_NOISE_GLITCH,
    SQUELCH_RSSI_GLITCH,
    SQUELCH_RSSI_NOISE,
    SQUELCH_RSSI,
};
const char *sqTypeNames[4] = {"RNG", "RG", "RN", "R"};
const char *deviationNames[] = {"", "+", "-"};

void RADIO_SetupRegisters(void) {
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
  BK4819_WriteRegister(BK4819_REG_7D, 0xE94F);
  BK4819_SetFrequency(Frequency);
  BK4819_SelectFilter(Frequency);
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, true);
  // BK4819_WriteRegister(BK4819_REG_48, 0xB3A8);
  BK4819_WriteRegister(
      BK4819_REG_48,
      (11u << 12) |    // ??? .. 0 ~ 15, doesn't seem to make any difference
          (1u << 10) | // AF Rx Gain-1
          (56 << 4) |  // AF Rx Gain-2
          (8 << 0));   // AF DAC Gain (after Gain-1 and Gain-2)

  BK4819_DisableScramble();

  BK4819_DisableVox();
  BK4819_DisableDTMF();

  BK4819_WriteRegister(BK4819_REG_3F, 0);
  /* BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_SQUELCH_FOUND |
                                          BK4819_REG_3F_SQUELCH_LOST); */
  BK4819_WriteRegister(0x40, (BK4819_ReadRegister(0x40) & ~(0b11111111111)) |
                                 0b10110101010 | (1 << 12));
  // BK4819_WriteRegister(0x40, (1 << 12) | (1450));
}

uint32_t GetScreenF(uint32_t f) {
  return f - upConverterValues[gSettings.upconverter];
}
uint32_t GetTuneF(uint32_t f) {
  return f + upConverterValues[gSettings.upconverter];
}

static void onVfoUpdate(void) {
  TaskRemove(RADIO_SaveCurrentVFO);
  TaskAdd("VFO save", RADIO_SaveCurrentVFO, 2000, false, 0);
}

static void onPresetUpdate(void) {
  TaskRemove(PRESETS_SaveCurrent);
  TaskAdd("Preset save", PRESETS_SaveCurrent, 2000, false, 0);
}

bool RADIO_IsBK1080Range(uint32_t f) { return f >= 6400000 && f <= 10800000; }

void toggleBK4819(bool on) {
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

void toggleBK1080(bool on) {
  if (on) {
    SYSTEM_DelayMs(10);
    AUDIO_ToggleSpeaker(true);
  } else {
    AUDIO_ToggleSpeaker(false);
    SYSTEM_DelayMs(10);
  }
}

void RADIO_ToggleRX(bool on) {
  if (gIsListening == on) {
    return;
  }
  gRedrawScreen = true;

  gIsListening = on;

  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, on);

  if (on) {
    if (gSettings.backlightOnSquelch != BL_SQL_OFF) {
      BACKLIGHT_On();
    }
  } else {
    if (gSettings.backlightOnSquelch == BL_SQL_OPEN) {
      BACKLIGHT_Toggle(false);
    }
  }

  /* if (isBK1080) {
    toggleBK1080(on);
  } else { */
  toggleBK4819(on);
  // }
}

void RADIO_EnableCxCSS(void) {
  switch (radio->tx.codeType) {
  /* case CODE_TYPE_DIGITAL:
  case CODE_TYPE_REVERSE_DIGITAL:
          BK4819_EnableCDCSS();
          break; */
  default:
    BK4819_EnableCTCSS();
    break;
  }

  SYSTEM_DelayMs(200);
}

static uint8_t calculateOutputPower(Preset *p, uint32_t Frequency) {
  uint8_t TxpLow = p->powCalib.s;
  uint8_t TxpMid = p->powCalib.m;
  uint8_t TxpHigh = p->powCalib.e;
  uint32_t LowerLimit = p->band.bounds.start;
  uint32_t UpperLimit = p->band.bounds.start;
  uint32_t Middle = LowerLimit + (UpperLimit - LowerLimit) / 2;

  if (Frequency <= LowerLimit) {
    return TxpLow;
  }

  if (UpperLimit <= Frequency) {
    return TxpHigh;
  }

  if (Frequency <= Middle) {
    TxpMid +=
        ((TxpMid - TxpLow) * (Frequency - LowerLimit)) / (Middle - LowerLimit);
    return TxpMid;
  }

  TxpMid += ((TxpHigh - TxpMid) * (Frequency - Middle)) / (UpperLimit - Middle);
  return TxpMid;
}

uint32_t RADIO_GetTXFEx(VFO *vfo, Preset *p) {
  uint32_t txF = vfo->rx.f;

  if (vfo->tx.f) {
    txF = vfo->tx.f;
  } else if (p->offset && p->offsetDir != OFFSET_NONE) {
    txF = vfo->rx.f + (p->offsetDir == OFFSET_PLUS ? p->offset : -p->offset);
  }

  return txF;
}

uint32_t RADIO_GetTXF(void) { return RADIO_GetTXFEx(radio, gCurrentPreset); }

TXState gTxState = TX_UNKNOWN;

void RADIO_ToggleTX(bool on) {
  if (gTxState == on) {
    return;
  }

  uint8_t power = 0;
  uint32_t txF = RADIO_GetTXF();

  if (on) {
    if (gSettings.upconverter) {
      gTxState = TX_DISABLED_UPCONVERTER;
      return;
    }

    Preset *txPreset = PRESET_ByFrequency(txF);

    if (!txPreset->allowTx) {
      gTxState = TX_DISABLED;
      return;
    }

    if (gBatteryPercent == 0) {
      gTxState = TX_BAT_LOW;
      return;
    }
    if (gChargingWithTypeC || gBatteryVoltage > 880) {
      gTxState = TX_VOL_HIGH;
      return;
    }
    power = calculateOutputPower(txPreset, txF);
    if (power > 0x91) {
      power = 0;
      gTxState = TX_POW_OVERDRIVE;
      return;
    }
    power >>= 2 - gCurrentPreset->power;
  }

  if (on) {
    RADIO_ToggleRX(false);

    BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, false);
    RADIO_SetupBandParams(&gCurrentPreset->band);

    BK4819_SetFrequency(txF);
    BK4819_SelectFilter(txF);

    BK4819_PrepareTransmit();

    SYSTEM_DelayMs(10);
    BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, true);
    SYSTEM_DelayMs(5);
    BK4819_SetupPowerAmplifier(power, txF);
    SYSTEM_DelayMs(10);
    BK4819_ExitSubAu();
  } else if (gTxState == TX_ON) {
    BK4819_ExitDTMF_TX(true);
    RADIO_EnableCxCSS();

    BK4819_SetupPowerAmplifier(0, 0);
    BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);
    BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, true);

    BK4819_SetFrequency(radio->rx.f);
    BK4819_SelectFilter(radio->rx.f);
    BK4819_RX_TurnOn();
  }

  BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, on);

  gTxState = on;
}

/* void RADIO_ToggleBK1080(bool on) {
  if (on == isBK1080) {
    return;
  }
  isBK1080 = on;

  if (isBK1080) {
    toggleBK4819(false);
    BK4819_Idle();
  } else {
    toggleBK1080(false);
    BK4819_RX_TurnOn();
  }
} */

void RADIO_SetModulationByPreset(void) {
  /* ModulationType mod = gCurrentPreset->band.modulation;
  if (mod == MOD_WFM) {
    if (RADIO_IsBK1080Range(radio->rx.f)) {
      RADIO_ToggleBK1080(true);
      return;
    }
    gCurrentPreset->band.modulation = MOD_FM;
  }
  RADIO_ToggleBK1080(false); */
  BK4819_SetModulation(gCurrentPreset->band.modulation);
  onPresetUpdate();
}

void RADIO_ToggleModulation(void) {
  if (gCurrentPreset->band.modulation == MOD_WFM) {
    gCurrentPreset->band.modulation = MOD_FM;
  } else {
    ++gCurrentPreset->band.modulation;
  }
  RADIO_SetModulationByPreset();
}

void RADIO_UpdateStep(bool inc) {
  uint8_t step = gCurrentPreset->band.step;
  IncDec8(&step, 0, STEP_200_0kHz, inc ? 1 : -1);
  gCurrentPreset->band.step = step;
  onPresetUpdate();
}

void RADIO_ToggleListeningBW(void) {
  if (gCurrentPreset->band.bw == BK4819_FILTER_BW_NARROWER) {
    gCurrentPreset->band.bw = BK4819_FILTER_BW_WIDE;
  } else {
    ++gCurrentPreset->band.bw;
  }

  BK4819_SetFilterBandwidth(gCurrentPreset->band.bw);
  onPresetUpdate();
}

void RADIO_ToggleTxPower(void) {
  if (gCurrentPreset->power == TX_POW_HIGH) {
    gCurrentPreset->power = TX_POW_LOW;
  } else {
    ++gCurrentPreset->power;
  }

  BK4819_SetFilterBandwidth(gCurrentPreset->band.bw);
  onPresetUpdate();
}

void RADIO_SetSquelchPure(uint32_t f, uint8_t sql) {
  BK4819_Squelch(sql, f, gSettings.sqlOpenTime, gSettings.sqlCloseTime);
}

void RADIO_TuneToPure(uint32_t f, bool precise) {
  LOOT_Replace(&gLoot[gSettings.activeVFO], f);
  /* if (isBK1080) {
    // BK1080_SetFrequency(f);
  } else { */
  BK4819_TuneTo(f, precise);
  // }
}

void RADIO_SetupByCurrentVFO(void) {
  uint32_t f = radio->rx.f;

  Preset *p = PRESET_ByFrequency(f);

  if (gCurrentPreset != p) {
    gCurrentPreset = p;
    gVFOPresets[gSettings.activeVFO] = gCurrentPreset;
    gSettings.activePreset = PRESET_GetCurrentIndex();

    RADIO_SetupBandParams(&gCurrentPreset->band);
    RADIO_ToggleRX(false); // to prevent muting when tune to new band
  }

  RADIO_TuneToPure(f, true);
}

// USE CASE: set vfo temporary for current app
void RADIO_TuneTo(uint32_t f) {
  radio->channel = -1;
  radio->tx.f = 0;
  radio->rx.f = f;
  RADIO_SetupByCurrentVFO();
}

// USE CASE: set vfo and use in another app
void RADIO_TuneToSave(uint32_t f) {
  RADIO_TuneTo(f);
  RADIO_SaveCurrentVFO();
  gCurrentPreset->lastUsedFreq = f;
  PRESETS_SaveCurrent();
}

void RADIO_SaveCurrentVFO(void) { VFOS_Save(gSettings.activeVFO, radio); }

void RADIO_VfoLoadCH(uint8_t i) {
  CH ch;
  CHANNELS_Load(gVFO[i].channel, &ch);
  CH2VFO(&ch, &gVFO[i]);
  strncpy(gVFONames[i], ch.name, 9);
}

void RADIO_SelectPreset(int8_t num) {
  PRESET_Select(num);
  RADIO_TuneTo(gCurrentPreset->lastUsedFreq);
}

void RADIO_SelectPresetSave(int8_t num) {
  PRESET_Select(num);
  PRESETS_SaveCurrent();
  RADIO_TuneToSave(gCurrentPreset->lastUsedFreq);
}

void RADIO_LoadCurrentVFO(void) {
  for (uint8_t i = 0; i < 2; ++i) {
    VFOS_Load(i, &gVFO[i]);
    if (gVFO[i].channel >= 0) {
      RADIO_VfoLoadCH(i);
    }
    gVFOPresets[i] = PRESET_ByFrequency(gVFO[i].rx.f);

    LOOT_Replace(&gLoot[i], gVFO[i].rx.f);
  }

  radio = &gVFO[gSettings.activeVFO];
  gCurrentLoot = &gLoot[gSettings.activeVFO];
  RADIO_SetupByCurrentVFO();
}

void RADIO_SetSquelch(uint8_t sq) {
  gCurrentPreset->band.squelch = sq;
  RADIO_SetSquelchPure(gCurrentPreset->band.bounds.start, sq);
  onPresetUpdate();
}

void RADIO_SetSquelchType(SquelchType t) {
  gCurrentPreset->band.squelchType = t;
  onPresetUpdate();
}

void RADIO_SetGain(uint8_t gainIndex) {
  BK4819_SetGain(gCurrentPreset->band.gainIndex = gainIndex);
  onPresetUpdate();
}

void RADIO_SetupBandParams(Band *b) {
  uint32_t fMid = b->bounds.start + (b->bounds.end - b->bounds.start) / 2;
  // BK4819_SelectFilter(fMid); // not needeed as evety BK4819_TuneTo() it works
  BK4819_SquelchType(b->squelchType);
  BK4819_Squelch(b->squelch, fMid, gSettings.sqlOpenTime,
                 gSettings.sqlCloseTime);
  BK4819_SetFilterBandwidth(b->bw);
  BK4819_SetModulation(b->modulation);
  BK4819_SetGain(b->gainIndex);
  // BK4819_RX_TurnOn(); // TODO: needeed?
}

uint16_t RADIO_GetRSSI(void) { return isBK1080 ? 128 : BK4819_GetRSSI(); }

static bool isSqOpenSimple(uint16_t r) {
  uint8_t band = radio->rx.f > SETTINGS_GetFilterBound() ? 1 : 0;
  uint8_t sq = gCurrentPreset->band.squelch;
  uint16_t ro = SQ[band][0][sq];
  uint16_t rc = SQ[band][1][sq];
  uint8_t no = SQ[band][2][sq];
  uint8_t nc = SQ[band][3][sq];
  uint8_t go = SQ[band][4][sq];
  uint8_t gc = SQ[band][5][sq];

  uint8_t n, g;

  bool open;

  switch (gCurrentPreset->band.squelchType) {
  case SQUELCH_RSSI_NOISE_GLITCH:
    n = BK4819_GetNoise();
    g = BK4819_GetGlitch();
    open = r >= ro && n <= no && g <= go;
    if (r < rc || n > nc || g > gc) {
      open = false;
    }
    break;
  case SQUELCH_RSSI_NOISE:
    n = BK4819_GetNoise();
    open = r >= ro && n <= no;
    if (r < rc || n > nc) {
      open = false;
    }
    break;
  case SQUELCH_RSSI_GLITCH:
    g = BK4819_GetGlitch();
    open = r >= ro && g <= go;
    if (r < rc || g > gc) {
      open = false;
    }
    break;
  case SQUELCH_RSSI:
    open = r >= ro;
    if (r < rc) {
      open = false;
    }
    break;
  }

  return open;
}

static uint32_t lastTailTone = 0;
Loot *RADIO_UpdateMeasurements(void) {
  Loot *msm = &gLoot[gSettings.activeVFO];
  msm->rssi = RADIO_GetRSSI();
  msm->open = isBK1080 ? true
                       : (gSettings.sqlOpenTime || gSettings.sqlCloseTime
                              ? BK4819_IsSquelchOpen()
                              : isSqOpenSimple(msm->rssi));

  while (BK4819_ReadRegister(BK4819_REG_0C) & 1u) {
    BK4819_WriteRegister(BK4819_REG_02, 0);

    uint16_t intBits = BK4819_ReadRegister(BK4819_REG_02);

    // MSG_StorePacket(intBits);

    if (intBits & BK4819_REG_02_CxCSS_TAIL) {
      msm->open = false;
      lastTailTone = Now();
    }
  }

  // else sql reopens
  if ((Now() - lastTailTone) < 250) {
    msm->open = false;
  }

  if (!gMonitorMode && gCurrentPreset->band.squelch != 0) {
    LOOT_Update(msm);
  }

  bool rx = msm->open;
  if (gTxState != TX_ON) {
    if (gMonitorMode) {
      rx = true;
    } else if (gSettings.noListen &&
               (gCurrentApp == APP_SPECTRUM || gCurrentApp == APP_ANALYZER)) {
      rx = false;
    } else if (gSettings.skipGarbageFrequencies &&
               (radio->rx.f % 1300000 == 0)) {
      rx = false;
    }
    RADIO_ToggleRX(rx);
  }
  return msm;
}

bool RADIO_UpdateMeasurementsEx(Loot *dest) {
  Loot *msm = &gLoot[gSettings.activeVFO];
  RADIO_UpdateMeasurements();
  LOOT_UpdateEx(dest, msm);
  return msm->open;
}

void RADIO_EnableToneDetection(void) {
  BK4819_SetCTCSSFrequency(670);
  BK4819_SetTailDetection(550);
  BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_CxCSS_TAIL/*  |
                                          BK4819_REG_3F_SQUELCH_LOST |
                                          BK4819_REG_3F_SQUELCH_FOUND */);
}

bool RADIO_TuneToCH(int32_t num) {
  if (CHANNELS_Existing(num)) {
    CH ch;
    CHANNELS_Load(num, &ch);
    CH2VFO(&ch, radio);
    strncpy(gVFONames[gSettings.activeVFO], ch.name, 9);
    radio->channel = num;
    onVfoUpdate();
    RADIO_SetupByCurrentVFO();
    return true;
  }
  return false;
}

void RADIO_NextCH(bool next) {
  int32_t i;
  if (radio->channel >= 0) {
    i = CHANNELS_Next(radio->channel, next);
    if (i > -1) {
      radio->channel = i;
      RADIO_VfoLoadCH(gSettings.activeVFO);
    }
    onVfoUpdate();
    RADIO_SetupByCurrentVFO();
    return;
  }

  i = radio->channel;

  if (!CHANNELS_Existing(radio->channel)) {
    i = CHANNELS_Next(radio->channel, true);
    if (i == -1) {
      return;
    }
  }

  RADIO_TuneToCH(i);
}

void RADIO_NextVFO(void) {
  gSettings.activeVFO = !gSettings.activeVFO;
  radio = &gVFO[gSettings.activeVFO];
  gCurrentLoot = &gLoot[gSettings.activeVFO];
  RADIO_SetupByCurrentVFO();
  RADIO_ToggleRX(false);
  SETTINGS_Save();
}

void RADIO_ToggleVfoMR(void) {
  if (radio->channel >= 0) {
    radio->channel = -1;
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

  if (radio->channel >= 0) {
    RADIO_NextCH(next);
    return;
  }

  Preset *nextPreset = PRESET_ByFrequency(radio->rx.f + dir);
  Band *nextBand = &nextPreset->band;
  uint32_t nextBandStep = StepFrequencyTable[nextBand->step];

  if (nextPreset != gCurrentPreset && nextPreset != &defaultPreset) {
    if (next) {
      RADIO_TuneTo(nextBand->bounds.start);
    } else {
      RADIO_TuneTo(nextBand->bounds.end - nextBand->bounds.end % nextBandStep);
    }
  } else {
    RADIO_TuneTo(radio->rx.f + nextBandStep * dir);
  }
  onVfoUpdate();
}

void RADIO_NextPresetFreq(bool next) {
  uint32_t steps = PRESETS_GetSteps(gCurrentPreset);
  uint32_t step = PRESETS_GetChannel(gCurrentPreset, radio->rx.f);
  IncDec32(&step, 0, steps, next ? 1 : -1);
  radio->rx.f = PRESETS_GetF(gCurrentPreset, step);
  RADIO_TuneToPure(radio->rx.f, true);
}

void RADIO_NextPresetFreqEx(bool next, bool precise) {
  uint32_t steps = PRESETS_GetSteps(gCurrentPreset);
  uint32_t step = PRESETS_GetChannel(gCurrentPreset, radio->rx.f);
  IncDec32(&step, 0, steps, next ? 1 : -1);
  radio->rx.f = PRESETS_GetF(gCurrentPreset, step);
  RADIO_TuneToPure(radio->rx.f, precise);
}
