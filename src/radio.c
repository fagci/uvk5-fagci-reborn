#include "radio.h"
#include "apps/apps.h"
#include "board.h"
#include "dcs.h"
#include "driver/audio.h"
#include "driver/backlight.h"
#include "driver/bk1080.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "driver/si473x.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "driver/uart.h"
#include "helper/adapter.h"
#include "helper/battery.h"
#include "helper/channels.h"
#include "helper/lootlist.h"
#include "helper/measurements.h"
#include "helper/presetlist.h"
#include "helper/vfos.h"
#include "misc.h"
#include "scheduler.h"
#include "settings.h"
#include "svc.h"
#include <string.h>

VFO *radio;
VFO gVFO[2] = {0};
Preset *gVFOPresets[2] = {0};

Loot gLoot[2] = {0};

char gVFONames[2][10] = {0};

bool gIsListening = false;
bool gMonitorMode = false;
TXState gTxState = TX_UNKNOWN;

static Radio oldRadio = RADIO_UNKNOWN;
static uint32_t lastTailTone = 0;
static uint32_t lastMsmUpdate = 0;
static bool toneFound = false;

const uint16_t StepFrequencyTable[15] = {
    2,   5,   25,  100,

    250, 500, 625, 833, 900, 1000, 1250, 2500, 5000, 10000, 50000,
};

const char *modulationTypeOptions[8] = {"FM",  "AM",  "LSB", "USB",
                                        "BYP", "RAW", "WFM", "Preset"};
const char *powerNames[4] = {"ULOW, LOW", "MID", "HIGH"};
const char *bwNames[5] = {"25k", "12.5k", "6.25k", "25k+"};
const char *bwNamesSiAMFM[5] = {"6k", "4k", "3k", "1k"};
const char *bwNamesSiSSB[5] = {"4k", "3k", "2.2k", "0.5k"};
const char *radioNames[4] = {"BK4819", "BK1080", "SI4732", "Preset"};
const char *shortRadioNames[4] = {"BK", "BC", "SI", "PR"};
const char *TX_STATE_NAMES[7] = {"TX Off",   "TX On",  "CHARGING", "BAT LOW",
                                 "DISABLED", "UPCONV", "HIGH POW"};

const SquelchType sqTypeValues[4] = {
    SQUELCH_RSSI_NOISE_GLITCH,
    SQUELCH_RSSI_GLITCH,
    SQUELCH_RSSI_NOISE,
    SQUELCH_RSSI,
};
const char *sqTypeNames[4] = {"RNG", "RG", "RN", "R"};
const char *deviationNames[] = {"", "+", "-"};

static const SI47XX_SsbFilterBW SI_BW_MAP_SSB[] = {
    [BK4819_FILTER_BW_WIDE] = SI47XX_SSB_BW_4_kHz,
    [BK4819_FILTER_BW_NARROW] = SI47XX_SSB_BW_3_kHz,
    [BK4819_FILTER_BW_NARROWER] = SI47XX_SSB_BW_2_2_kHz,
    [BK4819_FILTER_BW_SOMETHING] = SI47XX_SSB_BW_0_5_kHz,
};
static const SI47XX_FilterBW SI_BW_MAP_AMFM[] = {
    [BK4819_FILTER_BW_WIDE] = SI47XX_BW_6_kHz,
    [BK4819_FILTER_BW_NARROW] = SI47XX_BW_4_kHz,
    [BK4819_FILTER_BW_NARROWER] = SI47XX_BW_3_kHz,
    [BK4819_FILTER_BW_SOMETHING] = SI47XX_BW_1_kHz,
};

Radio RADIO_GetRadio() {
  return radio->radio == RADIO_UNKNOWN ? gCurrentPreset->radio : radio->radio;
}

ModulationType RADIO_GetModulation() {
  // return gCurrentPreset->band.modulation;
  return radio->modulation == MOD_PRST ? gCurrentPreset->band.modulation
                                       : radio->modulation;
}

const char *RADIO_GetBWName(BK4819_FilterBandwidth_t i) {
  switch (RADIO_GetRadio()) {
  case RADIO_SI4732:
    if (RADIO_IsSSB()) {
      return bwNamesSiSSB[i];
    }
    return bwNamesSiAMFM[i];
  default:
    return bwNames[i];
  }
}

void RADIO_SetupRegisters(void) {
  /* GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, false);
  BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false); */
  BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);
  BK4819_SetupPowerAmplifier(0, 0);

  // BK4819_SetFilterBandwidth(BK4819_FILTER_BW_WIDE);

  while (BK4819_ReadRegister(BK4819_REG_0C) & 1U) {
    BK4819_WriteRegister(BK4819_REG_02, 0);
    SYSTEM_DelayMs(1);
  }
  BK4819_WriteRegister(BK4819_REG_3F, 0);
  BK4819_WriteRegister(BK4819_REG_7D, 0xE94F | 10); // mic
  // TX
  // BK4819_WriteRegister(0x44, 38888);  // 300 resp TX
  BK4819_WriteRegister(0x74, 0xAF1F); // 3k resp TX

  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, true);
  BK4819_WriteRegister(
      BK4819_REG_48,
      (11u << 12) |    // ??? .. 0 ~ 15, doesn't seem to make any difference
          (1u << 10) | // AF Rx Gain-1
          (56 << 4) |  // AF Rx Gain-2
          (8 << 0));   // AF DAC Gain (after Gain-1 and Gain-2)

  BK4819_DisableScramble();
  BK4819_DisableVox();
  BK4819_DisableDTMF();

  // BK4819_WriteRegister(BK4819_REG_3F, 0);
  /* BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_SQUELCH_FOUND |
                                          BK4819_REG_3F_SQUELCH_LOST); */
  BK4819_WriteRegister(0x40, (BK4819_ReadRegister(0x40) & ~(0b11111111111)) |
                                 1300 | (1 << 12));
  // BK4819_WriteRegister(0x40, (1 << 12) | (1450));
}

static void setSI4732Modulation(ModulationType mod) {
  if (mod == MOD_AM) {
    SI47XX_SwitchMode(SI47XX_AM);
  } else if (mod == MOD_LSB) {
    SI47XX_SwitchMode(SI47XX_LSB);
  } else if (mod == MOD_USB) {
    SI47XX_SwitchMode(SI47XX_USB);
  } else {
    SI47XX_SwitchMode(SI47XX_FM);
  }
}

static void onVfoUpdate(void) {
  TaskRemove(RADIO_SaveCurrentVFO);
  TaskAdd("VFO sav", RADIO_SaveCurrentVFO, 2000, false, 0);
}

static void onPresetUpdate(void) {
  TaskRemove(PRESETS_SaveCurrent);
  TaskAdd("PRS sav", PRESETS_SaveCurrent, 2000, false, 0);
}

static void setupToneDetection() {
  uint16_t InterruptMask =
      BK4819_REG_3F_CxCSS_TAIL | BK4819_REG_3F_DTMF_5TONE_FOUND;
  if (gSettings.dtmfdecode) {
    BK4819_EnableDTMF();
  } else {
    BK4819_DisableDTMF();
  }
  switch (radio->rx.codeType) {
  case CODE_TYPE_DIGITAL:
  case CODE_TYPE_REVERSE_DIGITAL:
    // Log("DCS on");
    BK4819_SetCDCSSCodeWord(
        DCS_GetGolayCodeWord(radio->rx.codeType, radio->rx.code));
    InterruptMask |= BK4819_REG_3F_CDCSS_FOUND | BK4819_REG_3F_CDCSS_LOST;
    break;
  case CODE_TYPE_CONTINUOUS_TONE:
    // Log("CTCSS on");
    BK4819_SetCTCSSFrequency(CTCSS_Options[radio->rx.code]);
    InterruptMask |= BK4819_REG_3F_CTCSS_FOUND | BK4819_REG_3F_CTCSS_LOST;
    break;
  default:
    // Log("STE on");
    BK4819_SetCTCSSFrequency(670);
    BK4819_SetTailDetection(550);
    break;
  }
  BK4819_WriteRegister(BK4819_REG_3F, InterruptMask);
}

static bool isSimpleSql() {
  return gSettings.sqlOpenTime == 0 && gSettings.sqlCloseTime == 0;
}

static bool isSqOpenSimple(uint16_t r) {
  const uint8_t band = radio->rx.f > SETTINGS_GetFilterBound() ? 1 : 0;
  const uint8_t sq = gCurrentPreset->band.squelch;
  const uint16_t ro = SQ[band][0][sq];
  const uint16_t rc = SQ[band][1][sq];
  const uint8_t no = SQ[band][2][sq];
  const uint8_t nc = SQ[band][3][sq];
  const uint8_t go = SQ[band][4][sq];
  const uint8_t gc = SQ[band][5][sq];

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

static void toggleBK4819(bool on) {
  // Log("Toggle bk4819 audio %u", on);
  if (on) {
    BK4819_ToggleAFDAC(true);
    BK4819_ToggleAFBit(true);
    SYSTEM_DelayMs(8);
    AUDIO_ToggleSpeaker(true);
  } else {
    AUDIO_ToggleSpeaker(false);
    SYSTEM_DelayMs(8);
    BK4819_ToggleAFDAC(false);
    BK4819_ToggleAFBit(false);
  }
}

static void toggleBK1080SI4732(bool on) {
  // Log("Toggle bk1080si audio %u", on);
  if (on) {
    SYSTEM_DelayMs(8);
    AUDIO_ToggleSpeaker(true);
  } else {
    AUDIO_ToggleSpeaker(false);
    SYSTEM_DelayMs(8);
  }
}

static uint8_t calculateOutputPower(Preset *p) {
  uint8_t power_bias;

  switch (p->power) {
  case TX_POW_LOW:
    power_bias = p->powCalib.s;
    break;

  case TX_POW_MID:
    power_bias = p->powCalib.m;
    break;

  case TX_POW_HIGH:
    power_bias = p->powCalib.e;
    break;

  default:
    power_bias = p->powCalib.s;
    if (power_bias > 10)
      power_bias -= 10; // 10mw if Low=500mw
  }

  return power_bias;
}

static void sendEOT() {
  BK4819_ExitSubAu();
  switch (gSettings.roger) {
  case 1:
    BK4819_PlayRoger();
    break;
  case 2:
    BK4819_PlayRogerTiny();
    break;
  case 3:
    BK4819_PlayRogerUgly();
    break;
  default:
    break;
  }
  if (gSettings.ste) {
    SYSTEM_DelayMs(50);
    BK4819_GenTail(4);
    BK4819_WriteRegister(BK4819_REG_51, 0x9033);
    SYSTEM_DelayMs(200);
  }
  BK4819_ExitSubAu();
}

static void rxTurnOff(Radio r) {
  switch (r) {
  case RADIO_BK4819:
    BK4819_Idle();
    break;
  case RADIO_BK1080:
    BK1080_Mute(true);
    break;
  case RADIO_SI4732:
    if (gSettings.si4732PowerOff) {
      SI47XX_PowerDown();
    } else {
      SI47XX_SetVolume(0);
    }
    break;
  default:
    break;
  }
}

static void rxTurnOn(Radio r) {
  switch (r) {
  case RADIO_BK4819:
    BK4819_RX_TurnOn();
    break;
  case RADIO_BK1080:
    BK4819_Idle();
    BK1080_Mute(false);
    BK1080_Init(radio->rx.f, true);
    break;
  case RADIO_SI4732:
    BK4819_Idle();
    if (gSettings.si4732PowerOff || !isSi4732On) {
      if (RADIO_IsSSB()) {
        SI47XX_PatchPowerUp();
      } else {
        SI47XX_PowerUp();
      }
    } else {
      SI47XX_SetVolume(63);
    }
    break;
  default:
    break;
  }
}

uint32_t GetScreenF(uint32_t f) { return f - gSettings.upconverter; }

uint32_t GetTuneF(uint32_t f) { return f + gSettings.upconverter; }

bool RADIO_IsSSB() {
  ModulationType mod = RADIO_GetModulation();
  return mod == MOD_LSB || mod == MOD_USB;
}

void RADIO_ToggleRX(bool on) {
  if (gIsListening == on) {
    return;
  }
  gRedrawScreen = true;

  gIsListening = on;

  if (on) {
    if (gSettings.backlightOnSquelch != BL_SQL_OFF) {
      BACKLIGHT_On();
    }
  } else {
    if (gSettings.backlightOnSquelch == BL_SQL_OPEN) {
      BACKLIGHT_Toggle(false);
    }
  }

  Radio r = RADIO_GetRadio();
  if (r == RADIO_BK4819) {
    toggleBK4819(on);
  } else {
    toggleBK1080SI4732(on);
  }
}

void RADIO_EnableCxCSS(void) {
  switch (radio->tx.codeType) {
  case CODE_TYPE_CONTINUOUS_TONE:
    BK4819_SetCTCSSFrequency(CTCSS_Options[radio->tx.code]);
    break;
  case CODE_TYPE_DIGITAL:
  case CODE_TYPE_REVERSE_DIGITAL:
    BK4819_SetCDCSSCodeWord(
        DCS_GetGolayCodeWord(radio->tx.codeType, radio->tx.code));
    break;
  default:
    BK4819_ExitSubAu();
    break;
  }

  // SYSTEM_DelayMs(200);
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

TXState RADIO_GetTXState(uint32_t txF) {
  if (gSettings.upconverter) {
    return TX_DISABLED_UPCONVERTER;
  }

  Preset *txPreset = PRESET_ByFrequency(txF);

  if (!txPreset->allowTx || RADIO_GetRadio() != RADIO_BK4819 ||
      SVC_Running(SVC_FC)) {
    return TX_DISABLED;
  }

  if (gBatteryPercent == 0) {
    return TX_BAT_LOW;
  }
  if (gChargingWithTypeC || gBatteryVoltage > 880) {
    return TX_VOL_HIGH;
  }
  return TX_ON;
}

uint32_t RADIO_GetTxPower(uint32_t txF) {
  Preset *txPreset = PRESET_ByFrequency(txF);
  uint8_t power = calculateOutputPower(txPreset);
  if (power > 0x91) {
    power = 0x91;
  }
  // power >>= 2 - gCurrentPreset->power;
  return power;
}

void RADIO_ToggleTX(bool on) {
  uint32_t txF = RADIO_GetTXF();
  uint8_t power = RADIO_GetTxPower(txF);
  SVC_Toggle(SVC_FC, false, 0);
  RADIO_ToggleTXEX(on, txF, power, true);
}

void RADIO_ToggleTXEX(bool on, uint32_t txF, uint8_t power, bool paEnabled) {
  bool lastOn = gTxState == TX_ON;
  if (gTxState == on) {
    return;
  }

  gTxState = on ? RADIO_GetTXState(txF) : TX_UNKNOWN;

  if (gTxState == TX_ON) {
    RADIO_ToggleRX(false);

    BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, false);

    BK4819_TuneTo(txF, true);

    BOARD_ToggleRed(gSettings.brightness > 1);
    BK4819_PrepareTransmit();

    SYSTEM_DelayMs(10);
    BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, paEnabled);
    SYSTEM_DelayMs(5);
    BK4819_SetupPowerAmplifier(power, txF);
    SYSTEM_DelayMs(10);

    RADIO_EnableCxCSS();
  } else if (lastOn) {
    BK4819_ExitDTMF_TX(true); // also prepares to tx ste

    sendEOT();
    toggleBK1080SI4732(false);
    BOARD_ToggleRed(false);
    BK4819_TurnsOffTones_TurnsOnRX();

    BK4819_SetupPowerAmplifier(0, 0);
    BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);
    BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, true);

    setupToneDetection();
    BK4819_TuneTo(radio->rx.f, true);
  }
}

void RADIO_SetSquelchPure(uint32_t f, uint8_t sql) {
  BK4819_Squelch(sql, f, gSettings.sqlOpenTime, gSettings.sqlCloseTime);
}

void RADIO_TuneToPure(uint32_t f, bool precise) {
  LOOT_Replace(&gLoot[gSettings.activeVFO], f);
  Radio r = RADIO_GetRadio();
  // Log("Tune %s to %u", radioNames[r], f);
  switch (r) {
  case RADIO_BK4819:
    BK4819_TuneTo(f, precise);
    break;
  case RADIO_BK1080:
    BK1080_SetFrequency(f);
    break;
  case RADIO_SI4732:
    SI47XX_TuneTo(f);
    break;
  default:
    break;
  }
}

void RADIO_SwitchRadio() {
  Radio r = RADIO_GetRadio();
  if (oldRadio == r) {
    return;
  }
  // Log("Switch radio from %u to %u", oldRadio + 1, r + 1);
  rxTurnOff(oldRadio);
  rxTurnOn(r);
  oldRadio = r;
}

void RADIO_SetupByCurrentVFO(void) {
  uint32_t f = radio->rx.f;
  lastMsmUpdate = 0;
  PRESET_SelectByFrequency(f);
  gVFOPresets[gSettings.activeVFO] = gCurrentPreset;

  /* Log("SetupByCurrentVFO, p=%s, r=%s, f=%u", gCurrentPreset->band.name,
      radioNames[RADIO_GetRadio()], f); */

  RADIO_SwitchRadio();

  RADIO_SetupBandParams();
  setupToneDetection();

  RADIO_TuneToPure(f, !gMonitorMode); // todo: precise when old preset !=new?
}

// USE CASE: set vfo temporary for current app
void RADIO_TuneTo(uint32_t f) {
  if (radio->channel != -1) {
    radio->channel = -1;
    radio->radio = RADIO_UNKNOWN;
    radio->modulation = MOD_PRST;
  }
  radio->tx.f = 0;
  radio->rx.f = f;
  RADIO_SetupByCurrentVFO();
  setupToneDetection(); // note: idk where it will be
}

// USE CASE: set vfo and use in another app
void RADIO_TuneToSave(uint32_t f) {
  RADIO_TuneTo(f);
  RADIO_SaveCurrentVFO();
  gCurrentPreset->lastUsedFreq = f;
  PRESETS_SaveCurrent();
}

void RADIO_SaveCurrentVFO(void) { VFOS_Save(gSettings.activeVFO, radio); }

void RADIO_SelectPresetSave(int8_t num) {
  radio->radio = RADIO_UNKNOWN;
  radio->modulation = MOD_PRST;
  PRESET_Select(num);
  // PRESETS_SaveCurrent();
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
  gCurrentPreset->band.gainIndex = gainIndex;
  bool disableAGC = false;
  switch (RADIO_GetRadio()) {
  case RADIO_BK4819:
    BK4819_SetAGC(gCurrentPreset->band.modulation != MOD_AM, gainIndex);
    break;
  case RADIO_SI4732:
    // 0 - max gain
    // 26 - min gain
    gainIndex = ARRAY_SIZE(gainTable) - 1 - gainIndex;
    disableAGC = gainIndex != 0;
    gainIndex = ConvertDomain(gainIndex, 0, ARRAY_SIZE(gainTable) - 1, 0, 26);
    SI47XX_SetAutomaticGainControl(disableAGC, gainIndex);
    break;
  case RADIO_BK1080:
    break;
  default:
    break;
  }
  onPresetUpdate();
}

void RADIO_SetupBandParams() {
  // Log("RADIO_SetupBandParams");
  Band *b = &gCurrentPreset->band;
  uint32_t fMid = b->bounds.start + (b->bounds.end - b->bounds.start) / 2;
  ModulationType mod = RADIO_GetModulation();
  RADIO_SetGain(b->gainIndex);
  // Log("Set mod %s", modulationTypeOptions[mod]);
  switch (RADIO_GetRadio()) {
  case RADIO_BK4819:
    BK4819_SquelchType(b->squelchType);
    BK4819_Squelch(b->squelch, fMid, gSettings.sqlOpenTime,
                   gSettings.sqlCloseTime);
    BK4819_SetFilterBandwidth(b->bw);
    BK4819_SetModulation(mod);
    if (gSettings.scrambler) {
      BK4819_EnableScramble(gSettings.scrambler);
    } else {
      BK4819_DisableScramble();
    }
    // BK4819_RX_TurnOn(); /needed?
    break;
  case RADIO_BK1080:
    break;
  case RADIO_SI4732:
    if (mod == MOD_FM) {
      SI47XX_SetSeekFmLimits(b->bounds.start, b->bounds.end);
      SI47XX_SetSeekFmSpacing(StepFrequencyTable[b->step]);
    } else {
      if (mod == MOD_USB || mod == MOD_LSB) {
        SI47XX_SetSsbBandwidth(SI_BW_MAP_SSB[b->bw]);
      } else {
        SI47XX_SetBandwidth(SI_BW_MAP_AMFM[b->bw], true);
        SI47XX_SetSeekAmLimits(b->bounds.start, b->bounds.end);
        SI47XX_SetSeekAmSpacing(StepFrequencyTable[b->step]);
      }
    }

    setSI4732Modulation(mod);

    break;
  default:
    break;
  }
  // Log("RADIO_SetupBandParams end");
}

uint16_t RADIO_GetRSSI(void) {
  switch (RADIO_GetRadio()) {
  case RADIO_BK4819:
    return BK4819_GetRSSI();
  case RADIO_BK1080:
    return 0;
    return BK1080_GetRSSI();
  case RADIO_SI4732:
    return 0;
    RSQ_GET();
    return (160 + (rsqStatus.resp.RSSI - 107)) << 1;
  default:
    return 128;
  }
}

bool RADIO_IsSquelchOpen(Loot *msm) {
  if (RADIO_GetRadio() == RADIO_BK4819) {
    if (isSimpleSql()) {
      return isSqOpenSimple(msm->rssi);
    } else {
      return BK4819_IsSquelchOpen();
    }
  }
  return true;
}

Loot *RADIO_UpdateMeasurements(void) {
  Loot *msm = &gLoot[gSettings.activeVFO];
  if (RADIO_GetRadio() == RADIO_SI4732 && SVC_Running(SVC_SCAN)) {
    bool valid = false;
    uint32_t f = SI47XX_getFrequency(&valid);
    radio->rx.f = f;
    gRedrawScreen = true;
    if (valid) {
      SVC_Toggle(SVC_SCAN, false, 0);
    }
  }
  if (RADIO_GetRadio() != RADIO_BK4819 && Now() - lastMsmUpdate <= 1000) {
    return msm;
  }
  lastMsmUpdate = Now();
  msm->rssi = RADIO_GetRSSI();
  msm->open = RADIO_IsSquelchOpen(msm);
  if (radio->rx.codeType == CODE_TYPE_OFF) {
    toneFound = true;
  }

  if (RADIO_GetRadio() == RADIO_BK4819) {
    while (BK4819_ReadRegister(BK4819_REG_0C) & 1u) {
      BK4819_WriteRegister(BK4819_REG_02, 0);

      uint16_t intBits = BK4819_ReadRegister(BK4819_REG_02);

      if ((intBits & BK4819_REG_02_CxCSS_TAIL) ||
          (intBits & BK4819_REG_02_CTCSS_FOUND) ||
          (intBits & BK4819_REG_02_CDCSS_FOUND)) {
        // Log("Tail tone or ctcss/dcs found");
        msm->open = false;
        toneFound = false;
        lastTailTone = Now();
      }
      if ((intBits & BK4819_REG_02_CTCSS_LOST) ||
          (intBits & BK4819_REG_02_CDCSS_LOST)) {
        // Log("ctcss/dcs lost");
        msm->open = true;
        toneFound = true;
      }

      /* if (intBits & BK4819_REG_02_DTMF_5TONE_FOUND) {
        uint8_t code = BK4819_GetDTMF_5TONE_Code();
        Log("DTMF: %u", code);
      } */
    }
    // else sql reopens
    if (!toneFound || (Now() - lastTailTone) < 250) {
      msm->open = false;
    }
  }
  if (RADIO_GetRadio() == RADIO_BK4819) {
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
               (radio->rx.f % 1300000 == 0) &&
               RADIO_GetRadio() == RADIO_BK4819) {
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

void RADIO_VfoLoadCH(uint8_t i) {
  CH ch;
  CHANNELS_Load(gVFO[i].channel, &ch);
  CH2VFO(&ch, &gVFO[i]);
  strncpy(gVFONames[i], ch.name, 9);
}

bool RADIO_TuneToCH(int32_t num) {
  if (CHANNELS_Existing(num)) {
    radio->channel = num;
    RADIO_VfoLoadCH(gSettings.activeVFO);
    onVfoUpdate();
    RADIO_SetupByCurrentVFO();
    return true;
  }
  return false;
}

void RADIO_NextCH(bool next) {
  RADIO_TuneToCH(CHANNELS_Next(radio->channel, next));
}

void RADIO_NextVFO(void) {
  gSettings.activeVFO = !gSettings.activeVFO;
  radio = &gVFO[gSettings.activeVFO];
  RADIO_SetupByCurrentVFO();
  SETTINGS_Save();
}

void RADIO_ToggleVfoMR(void) {
  if (radio->channel >= 0) {
    radio->channel += 1; // 0 -> 1
    radio->channel *= -1;
  } else {
    radio->channel *= -1;
    radio->channel -= 1; // 1 -> 0
    if (CHANNELS_Existing(radio->channel)) {
      RADIO_TuneToCH(radio->channel);
    } else {
      RADIO_NextCH(true);
    }
  }
  RADIO_SaveCurrentVFO();
}

void RADIO_UpdateSquelchLevel(bool next) {
  uint8_t sq = gCurrentPreset->band.squelch;
  IncDec8(&sq, 0, 10, next ? 1 : -1);
  RADIO_SetSquelch(sq);
}

void RADIO_NextFreqNoClicks(bool next) {
  const int8_t dir = next ? 1 : -1;

  if (radio->channel >= 0) {
    RADIO_NextCH(next);
    return;
  }

  Preset *nextPreset = PRESET_ByFrequency(radio->rx.f + dir);
  Band *nextBand = &nextPreset->band;
  uint32_t nextBandStep = StepFrequencyTable[nextBand->step];

  uint32_t f = radio->rx.f + nextBandStep * dir;
  if (nextPreset != gCurrentPreset && nextPreset != &defaultPreset &&
      !PRESET_InRange(f, nextPreset)) {
    if (next) {
      RADIO_TuneTo(nextBand->bounds.start);
    } else {
      RADIO_TuneTo(nextBand->bounds.end - nextBand->bounds.end % nextBandStep);
    }
  } else {
    f = PRESETS_GetF(nextPreset, PRESETS_GetChannel(nextPreset, f));
    radio->channel = -1;
    radio->tx.f = 0;
    radio->rx.f = f;
    RADIO_SetupByCurrentVFO();
  }
  onVfoUpdate();
}

static void selectPreset(bool next) {
  if (gSettings.crossBandScan) {
    uint8_t index = gSettings.activePreset;
    uint8_t sl = gSettings.currentScanlist;
    uint8_t scanlistMask = 1 << sl;
    PRESETS_SelectPresetRelative(next);
    while (gSettings.activePreset != index) {
      if (sl == 15 ||
          (gCurrentPreset->memoryBanks & scanlistMask) == scanlistMask) {
        return;
      }
      PRESETS_SelectPresetRelative(next);
    }
  }
}

bool RADIO_NextPresetFreqXBandEx(bool next, bool tune, bool precise) {
  uint32_t steps = PRESETS_GetSteps(gCurrentPreset);
  int64_t step = PRESETS_GetChannel(gCurrentPreset, radio->rx.f);
  bool switchBand = false;

  if (next) {
    step++;
  } else {
    step--;
  }

  if (step < 0) {
    // get previous preset
    switchBand = true;
    selectPreset(false);
    steps = PRESETS_GetSteps(gCurrentPreset);
    step = steps - 1;
  } else if (step >= steps) {
    // get next preset
    switchBand = true;
    selectPreset(true);
    step = 0;
  }
  radio->rx.f = PRESETS_GetF(gCurrentPreset, step);
  if (tune) {
    RADIO_TuneToPure(radio->rx.f, precise);
  }
  return switchBand;
}

void RADIO_NextPresetFreqXBand(bool next) {
  RADIO_NextPresetFreqXBandEx(next, true, true);
}

static ModulationType MODS_BK4819[] = {
    MOD_FM, MOD_AM, MOD_USB, MOD_BYP, MOD_RAW, MOD_WFM,
};

static ModulationType MODS_BK1080[] = {
    MOD_WFM,
};

static ModulationType MODS_SI4732_HF[] = {
    MOD_AM,
    MOD_LSB,
    MOD_USB,
};

static ModulationType MODS_SI4732_WFM[] = {
    MOD_WFM,
};

static int8_t indexOf(ModulationType *arr, uint8_t n, ModulationType t) {
  for (uint8_t i = 0; i < n; ++i) {
    if (arr[i] == t) {
      return i;
    }
  }
  return 0;
}

static ModulationType getNextModulation() {
  const Radio r = RADIO_GetRadio();
  uint8_t sz;
  ModulationType *items;

  if (r == RADIO_BK4819) {
    items = MODS_BK4819;
    sz = ARRAY_SIZE(MODS_BK4819);
  } else if (r == RADIO_BK1080) {
    items = MODS_BK1080;
    sz = ARRAY_SIZE(MODS_BK1080);
  } else {
    // si4732
    if (radio->rx.f <= 3000000) {
      items = MODS_SI4732_HF;
      sz = ARRAY_SIZE(MODS_SI4732_HF);
    } else {
      items = MODS_SI4732_WFM;
      sz = ARRAY_SIZE(MODS_SI4732_WFM);
    }
  }
  int8_t curIndex =
      indexOf(items, ARRAY_SIZE(MODS_BK4819), RADIO_GetModulation());
  if (curIndex >= 0) {
    IncDec8(&curIndex, 0, sz, 1);
    if (items[curIndex] == gCurrentPreset->band.modulation) {
      return MOD_PRST;
    }
  } else {
    return items[0];
  }
  return items[curIndex];
}

void RADIO_ToggleModulation(void) {
  radio->modulation = getNextModulation();

  // NOTE: for right BW after switching from WFM to another
  RADIO_SetupBandParams();
  onVfoUpdate();
}

void RADIO_UpdateStep(bool inc) {
  uint8_t step = gCurrentPreset->band.step;
  IncDec8(&step, 0, STEP_500_0kHz, inc ? 1 : -1);
  gCurrentPreset->band.step = step;
  onPresetUpdate();
}

void RADIO_ToggleListeningBW(void) {
  if (gCurrentPreset->band.bw == BK4819_FILTER_BW_SOMETHING) {
    gCurrentPreset->band.bw = BK4819_FILTER_BW_WIDE;
  } else {
    ++gCurrentPreset->band.bw;
  }

  BK4819_SetFilterBandwidth(gCurrentPreset->band.bw);
  onPresetUpdate();
}

void RADIO_ToggleTxPower(void) {
  if (gCurrentPreset->power == TX_POW_HIGH) {
    gCurrentPreset->power = TX_POW_ULOW;
  } else {
    ++gCurrentPreset->power;
  }

  onPresetUpdate();
}
