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
#include "external/printf/printf.h"
#include "helper/battery.h"
#include "helper/channels.h"
#include "helper/lootlist.h"
#include "helper/measurements.h"
#include "misc.h"
#include "scheduler.h"
#include "settings.h"
#include "svc.h"

CH *radio;
VFO gVFO[2] = {
    (VFO){
        .name = "VFO1",
        .rxF = 14550000,
    },
    (VFO){
        .name = "VFO2",
        .rxF = 43307500,
    },
};

Loot gLoot[2] = {0};

bool gIsListening = false;
bool gMonitorMode = false;
TXState gTxState = TX_UNKNOWN;
bool gShowAllRSSI = false;

bool hasSi = false;

static uint8_t oldRadio = 255;

const uint16_t StepFrequencyTable[15] = {
    2,   5,   50,  100,

    250, 500, 625, 833, 900, 1000, 1250, 2500, 5000, 10000, 50000,
};

const char *modulationTypeOptions[8] = {"FM",  "AM",  "LSB", "USB",
                                        "BYP", "RAW", "WFM"};
const char *powerNames[4] = {"ULOW, LOW", "MID", "HIGH"};
const char *bwNames[10] = {"W26k", "W23k", "W20k", "W17k", "W14k", "W12k",
                           "N10k", "N9k",  "U7K",  "U6K"

};
const char *bwNamesSiAMFM[7] = {
    [BK4819_FILTER_BW_6k] = "1k",  [BK4819_FILTER_BW_7k] = "1.8k",
    [BK4819_FILTER_BW_9k] = "2k",  [BK4819_FILTER_BW_10k] = "2.5k",
    [BK4819_FILTER_BW_12k] = "3k", [BK4819_FILTER_BW_14k] = "4k",
    [BK4819_FILTER_BW_17k] = "6k",
};
const char *bwNamesSiSSB[6] = {
    [BK4819_FILTER_BW_6k] = "0.5k", [BK4819_FILTER_BW_7k] = "1.0k",
    [BK4819_FILTER_BW_9k] = "1.2k", [BK4819_FILTER_BW_10k] = "2.2k",
    [BK4819_FILTER_BW_12k] = "3k",  [BK4819_FILTER_BW_14k] = "4k",

};
const char *radioNames[4] = {"BK4819", "BK1080", "SI4732"};
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
    [BK4819_FILTER_BW_6k] = SI47XX_SSB_BW_0_5_kHz,
    [BK4819_FILTER_BW_7k] = SI47XX_SSB_BW_1_0_kHz,
    [BK4819_FILTER_BW_9k] = SI47XX_SSB_BW_1_2_kHz,
    [BK4819_FILTER_BW_10k] = SI47XX_SSB_BW_2_2_kHz,
    [BK4819_FILTER_BW_12k] = SI47XX_SSB_BW_3_kHz,
    [BK4819_FILTER_BW_14k] = SI47XX_SSB_BW_4_kHz,
};
static const SI47XX_FilterBW SI_BW_MAP_AMFM[] = {
    [BK4819_FILTER_BW_6k] = SI47XX_BW_1_kHz,
    [BK4819_FILTER_BW_7k] = SI47XX_BW_1_8_kHz,
    [BK4819_FILTER_BW_9k] = SI47XX_BW_2_kHz,
    [BK4819_FILTER_BW_10k] = SI47XX_BW_2_5_kHz,
    [BK4819_FILTER_BW_12k] = SI47XX_BW_3_kHz,
    [BK4819_FILTER_BW_14k] = SI47XX_BW_4_kHz,
    [BK4819_FILTER_BW_17k] = SI47XX_BW_6_kHz,
};

static ModulationType MODS_BK4819[] = {
    MOD_FM,
    MOD_AM,
    MOD_USB,
    MOD_WFM,
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

static int8_t indexOfMod(ModulationType *arr, uint8_t n, ModulationType t) {
  for (uint8_t i = 0; i < n; ++i) {
    if (arr[i] == t) {
      return i;
    }
  }
  return 0;
}

static ModulationType getNextModulation() {
  uint8_t sz;
  ModulationType *items;

  switch (RADIO_GetRadio()) {
  case RADIO_BK4819:
    items = MODS_BK4819;
    sz = ARRAY_SIZE(MODS_BK4819);
    break;
  case RADIO_BK1080:
    items = MODS_BK1080;
    sz = ARRAY_SIZE(MODS_BK1080);
    break;
  default:
    if (radio->rxF <= 3000000) {
      items = MODS_SI4732_HF;
      sz = ARRAY_SIZE(MODS_SI4732_HF);
    } else {
      items = MODS_SI4732_WFM;
      sz = ARRAY_SIZE(MODS_SI4732_WFM);
    }
    break;
  }
  int8_t curIndex =
      indexOfMod(items, ARRAY_SIZE(MODS_BK4819), RADIO_GetModulation());
  if (curIndex >= 0) {
    IncDecI8(&curIndex, 0, sz, 1);
  } else {
    return items[0];
  }
  return items[curIndex];
}

static void loadVFO(uint8_t num) {
  CHANNELS_Load(CHANNELS_GetCountMax() - 2 + num, &gVFO[num]);
}

static void saveVFO(uint8_t num) {
  CHANNELS_Save(CHANNELS_GetCountMax() - 2 + num, &gVFO[num]);
}

Radio RADIO_Selector(uint32_t freq, ModulationType mod) {
  if ((freq >= BK1080_F_MIN && freq <= BK1080_F_MAX) && mod == MOD_WFM) {
    return hasSi ? RADIO_SI4732 : RADIO_BK1080;
  }

  if (hasSi && freq <= SI47XX_F_MAX &&
      (mod == MOD_AM || (isPatchPresent && RADIO_IsSSB()))) {
    return RADIO_SI4732;
  }

  return RADIO_BK4819;
}

inline Radio RADIO_GetRadio() { return radio->radio; }

ModulationType RADIO_GetModulation() { return radio->modulation; }

const char *RADIO_GetBWName(Radio r, BK4819_FilterBandwidth_t i) {
  switch (r) {
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
  BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);
  BK4819_SetupPowerAmplifier(0, 0); // 0 is default, but...

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

  BK4819_DisableDTMF();

  BK4819_WriteRegister(0x40, (BK4819_ReadRegister(0x40) & ~(0x7FF)) | 1000 |
                                 (1 << 12));
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
  Log("VFO sav in 2s...");
  TaskRemove(RADIO_SaveCurrentVFO);
  TaskAdd("VFO sav", RADIO_SaveCurrentVFO, 2000, false, 0);
}

static void onPresetUpdate(void) {
  Log("PRESET sav in 2s...");
  TaskRemove(PRESETS_SaveCurrent);
  TaskAdd("PRS sav", PRESETS_SaveCurrent, 2000, false, 0);
}

static void setupToneDetection() {
  Log("!!! Setup tone detection");
  uint16_t InterruptMask =
      BK4819_REG_3F_CxCSS_TAIL | BK4819_REG_3F_DTMF_5TONE_FOUND;
  if (gSettings.dtmfdecode) {
    BK4819_EnableDTMF();
    BK4819_WriteRegister(BK4819_REG_3F, 0x0800); // FIXME: RM
    Log("!!! Setup DTMF");
    return;
  } else {
    BK4819_DisableDTMF();
  }
  switch (radio->code.rx.type) {
  case CODE_TYPE_DIGITAL:
  case CODE_TYPE_REVERSE_DIGITAL:
    Log("DCS on");
    BK4819_SetCDCSSCodeWord(
        DCS_GetGolayCodeWord(radio->code.rx.type, radio->code.rx.value));
    InterruptMask |= BK4819_REG_3F_CDCSS_FOUND | BK4819_REG_3F_CDCSS_LOST;
    break;
  case CODE_TYPE_CONTINUOUS_TONE:
    Log("CTCSS on");
    BK4819_SetCTCSSFrequency(CTCSS_Options[radio->code.rx.value]);
    InterruptMask |= BK4819_REG_3F_CTCSS_FOUND | BK4819_REG_3F_CTCSS_LOST;
    break;
  default:
    Log("STE on");
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
  SQL sq = GetSql(radio->squelch.value);

  uint8_t n, g;

  bool open;

  switch (radio->squelch.type) {
  case SQUELCH_RSSI_NOISE_GLITCH:
    n = BK4819_GetNoise();
    g = BK4819_GetGlitch();
    open = r >= sq.ro && n <= sq.no && g <= sq.go;
    if (r < sq.rc || n > sq.nc || g > sq.gc) {
      open = false;
    }
    break;
  case SQUELCH_RSSI_NOISE:
    n = BK4819_GetNoise();
    open = r >= sq.ro && n <= sq.no;
    if (r < sq.rc || n > sq.nc) {
      open = false;
    }
    break;
  case SQUELCH_RSSI_GLITCH:
    g = BK4819_GetGlitch();
    open = r >= sq.ro && g <= sq.go;
    if (r < sq.rc || g > sq.gc) {
      open = false;
    }
    break;
  case SQUELCH_RSSI:
    open = r >= sq.ro;
    if (r < sq.rc) {
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
    power_bias = p->misc.powCalib.s;
    break;

  case TX_POW_MID:
    power_bias = p->misc.powCalib.m;
    break;

  case TX_POW_HIGH:
    power_bias = p->misc.powCalib.e;
    break;

  default:
    power_bias = p->misc.powCalib.s;
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
    BK1080_Init(radio->rxF, true);
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
  switch (radio->code.tx.type) {
  case CODE_TYPE_CONTINUOUS_TONE:
    BK4819_SetCTCSSFrequency(CTCSS_Options[radio->code.tx.value]);
    break;
  case CODE_TYPE_DIGITAL:
  case CODE_TYPE_REVERSE_DIGITAL:
    BK4819_SetCDCSSCodeWord(
        DCS_GetGolayCodeWord(radio->code.tx.type, radio->code.tx.value));
    break;
  default:
    BK4819_ExitSubAu();
    break;
  }

  // SYSTEM_DelayMs(200);
}

uint32_t RADIO_GetTXFEx(VFO *vfo, Preset *p) {
  uint32_t txF = vfo->rxF;

  if (vfo->txF && p->offsetDir == OFFSET_FREQ) {
    txF = vfo->txF;
  } else if (p->txF && p->offsetDir != OFFSET_NONE) {
    txF = vfo->rxF + (p->offsetDir == OFFSET_PLUS ? p->txF : -p->txF);
  }

  return txF;
}

uint32_t RADIO_GetTXF(void) { return RADIO_GetTXFEx(radio, &gCurrentPreset); }

TXState RADIO_GetTXState(uint32_t txF) {
  if (gSettings.upconverter) {
    return TX_DISABLED_UPCONVERTER;
  }

  const Preset txPreset = PRESET_ByFrequency(txF);

  if (!txPreset.allowTx || RADIO_GetRadio() != RADIO_BK4819 ||
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
  Preset txPreset = PRESET_ByFrequency(txF);
  uint8_t power = calculateOutputPower(&txPreset);
  if (power > 0x91) {
    power = 0x91;
  }
  // power >>= 2 - gCurrentPreset.power;
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
    BK4819_TuneTo(radio->rxF, true);
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
  // Radio r = RADIO_GetRadio();
  radio->radio = RADIO_Selector(radio->rxF, radio->modulation);
  if (oldRadio == radio->radio) {
    return;
  }
  /* Log("Switch radio from %s to %s",
      oldRadio == UINT8_MAX ? "-" : radioNames[oldRadio], radioNames[r]); */
  rxTurnOff(oldRadio);
  rxTurnOn(radio->radio);
  oldRadio = radio->radio;
}

void RADIO_SetupByCurrentVFO(void) {
  Log("Setup by current VFO (f=%u, radio=%u)", radio->rxF, radio->radio);
  uint32_t f = radio->rxF;
  PRESET_SelectByFrequency(f);

  // Log("Switch radio");
  RADIO_SwitchRadio();

  RADIO_SetupBandParams();
  setupToneDetection();

  RADIO_TuneToPure(f, !gMonitorMode); // todo: precise when old preset !=new?
}

// USE CASE: set vfo temporary for current app
void RADIO_TuneTo(uint32_t f) {
  if (radio->channel != -1) {
    radio->channel = -1;
    snprintf(radio->name, 5, "VFO%u", gSettings.activeVFO + 1);
  }
  radio->txF = 0;
  radio->rxF = f;
  RADIO_SetupByCurrentVFO();
  setupToneDetection(); // note: idk where it will be
}

// USE CASE: set vfo and use in another app
void RADIO_TuneToSave(uint32_t f) {
  RADIO_TuneTo(f);
  gCurrentPreset.misc.lastUsedFreq = f;
  RADIO_SaveCurrentVFO();
  PRESETS_SaveCurrent();
}

void RADIO_SaveCurrentVFO(void) {
  int16_t vfoChNum = CHANNELS_GetCountMax() - 2 + gSettings.activeVFO;
  int16_t chToSave = radio->channel;
  if (chToSave >= 0) {
    // save only active channel number
    // to load it instead of full VFO
    // and to prevent overwrite VFO with MR
    VFO oldVfo;
    CHANNELS_Load(vfoChNum, &oldVfo);
    oldVfo.channel = chToSave;
    CHANNELS_Save(vfoChNum, &oldVfo);
    return;
  }
  CHANNELS_Save(vfoChNum, radio);
}

void RADIO_SelectPresetSave(int8_t num) {
  // TODO: copy settings from preset
  PRESET_Select(num);
  if (PRESET_InRange(radio->misc.lastUsedFreq, gCurrentPreset)) {
    RADIO_TuneToSave(radio->misc.lastUsedFreq);
  } else {
    RADIO_TuneToSave(radio->rxF);
  }
}

void RADIO_LoadCurrentVFO(void) {
  for (uint8_t i = 0; i < 2; ++i) {
    loadVFO(i);
    Log("gVFO(%u)= (f=%u, radio=%u)", i + 1, gVFO[i].rxF, gVFO[i].radio);
    if (gVFO[i].channel >= 0) {
      RADIO_VfoLoadCH(i);
    }

    LOOT_Replace(&gLoot[i], gVFO[i].rxF);
  }

  radio = &gVFO[gSettings.activeVFO];
  RADIO_SetupByCurrentVFO();
}

void RADIO_SetSquelch(uint8_t sq) {
  radio->squelch.value = sq;
  RADIO_SetSquelchPure(radio->rxF, sq);
  onVfoUpdate();
}

void RADIO_SetSquelchType(SquelchType t) {
  radio->squelch.type = t;
  onVfoUpdate();
}

void RADIO_SetGain(uint8_t gainIndex) {
  radio->gainIndex = gainIndex;
  bool disableAGC;
  switch (RADIO_GetRadio()) {
  case RADIO_BK4819:
    BK4819_SetAGC(radio->modulation != MOD_AM, gainIndex);
    break;
  case RADIO_SI4732:
    // 0 - max gain
    // 26 - min gain
    disableAGC = gainIndex != AUTO_GAIN_INDEX;
    gainIndex = ARRAY_SIZE(gainTable) - 1 - gainIndex;
    gainIndex = ConvertDomain(gainIndex, 0, ARRAY_SIZE(gainTable) - 1, 0, 26);
    SI47XX_SetAutomaticGainControl(disableAGC, gainIndex);
    break;
  case RADIO_BK1080:
    break;
  default:
    break;
  }
}

void RADIO_SetFilterBandwidth(BK4819_FilterBandwidth_t bw) {
  ModulationType mod = RADIO_GetModulation();
  switch (RADIO_GetRadio()) {
  case RADIO_BK4819:
    BK4819_SetFilterBandwidth(bw);
    break;
  case RADIO_BK1080:
    break;
  case RADIO_SI4732:
    if (mod == MOD_USB || mod == MOD_LSB) {
      SI47XX_SetSsbBandwidth(SI_BW_MAP_SSB[bw]);
    } else {
      SI47XX_SetBandwidth(SI_BW_MAP_AMFM[bw], true);
    }
    break;
  default:
    break;
  }
}

void RADIO_SetupBandParams() {
  Log("RADIO_SetupBandParams");
  ModulationType mod = RADIO_GetModulation();
  RADIO_SetGain(radio->gainIndex);
  // Log("Set mod %s", modulationTypeOptions[mod]);
  RADIO_SetFilterBandwidth(radio->bw);
  switch (RADIO_GetRadio()) {
  case RADIO_BK4819:
    BK4819_SquelchType(radio->squelch.type);
    BK4819_Squelch(radio->squelch.value, radio->rxF, gSettings.sqlOpenTime,
                   gSettings.sqlCloseTime);
    BK4819_SetModulation(mod);
    if (gSettings.scrambler) {
      BK4819_EnableScramble(gSettings.scrambler);
    } else {
      BK4819_DisableScramble();
    }
    break;
  case RADIO_BK1080:
    break;
  case RADIO_SI4732:
    if (mod == MOD_FM) {
      SI47XX_SetSeekFmLimits(radio->rxF, radio->txF);
      SI47XX_SetSeekFmSpacing(StepFrequencyTable[radio->step]);
    } else {
      if (mod == MOD_AM) {
        SI47XX_SetSeekAmLimits(radio->rxF, radio->txF);
        SI47XX_SetSeekAmSpacing(StepFrequencyTable[radio->step]);
      }
    }

    setSI4732Modulation(mod);

    break;
  default:
    break;
  }
  Log("RADIO_SetupBandParams end");
}

uint16_t RADIO_GetRSSI(void) {
  switch (RADIO_GetRadio()) {
  case RADIO_BK4819:
    return BK4819_GetRSSI();
  case RADIO_BK1080:
    return gShowAllRSSI ? BK1080_GetRSSI() : 0;
  case RADIO_SI4732:
    if (gShowAllRSSI) {
      RSQ_GET();
      return ConvertDomain(rsqStatus.resp.RSSI, 0, 64, 30, 346);
    }
    return 0;
  default:
    return 128;
  }
}

uint8_t RADIO_GetSNR(void) {
  switch (RADIO_GetRadio()) {
  case RADIO_BK4819:
    return ConvertDomain(BK4819_GetSNR(), 24, 170, 0, 30);
  case RADIO_BK1080:
    return gShowAllRSSI ? BK1080_GetSNR() : 0;
  case RADIO_SI4732:
    if (gShowAllRSSI) {
      RSQ_GET();
      return rsqStatus.resp.SNR;
    }
    return 0;
  default:
    return 0;
  }
}

uint16_t RADIO_GetS() {
  uint8_t snr = RADIO_GetSNR();
  switch (RADIO_GetRadio()) {
  case RADIO_BK4819:
    return ConvertDomain(snr, 0, 137, 0, 13);
  case RADIO_BK1080:
    return ConvertDomain(snr, 0, 137, 0, 13);
  case RADIO_SI4732:
    return ConvertDomain(snr, 0, 30, 0, 13);
  default:
    return 0;
  }
}

bool RADIO_IsSquelchOpen(const Loot *msm) {
  if (RADIO_GetRadio() == RADIO_BK4819) {
    if (isSimpleSql()) {
      return isSqOpenSimple(msm->rssi);
    } else {
      return BK4819_IsSquelchOpen();
    }
  }
  return true;
}

void RADIO_VfoLoadCH(uint8_t i) {
  int16_t chNum = gVFO[i].channel;
  CHANNELS_Load(gVFO[i].channel, &gVFO[i]);
  gVFO[i].meta.type = TYPE_VFO;
  gVFO[i].channel = chNum;
}

// TODO:
// keep CH in memory, not save in VFO, only save channel
bool RADIO_TuneToCH(int32_t num) {
  if (CHANNELS_Existing(num)) {
    radio->channel = num;
    RADIO_VfoLoadCH(gSettings.activeVFO);
    RADIO_SaveCurrentVFO();
    RADIO_SetupByCurrentVFO();
    return true;
  }
  radio->channel = -1;
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
    loadVFO(gSettings.activeVFO);
    radio->channel += 1; // 0 -> 1
    radio->channel *= -1;
    saveVFO(gSettings.activeVFO);
    RADIO_SetupByCurrentVFO();
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
  uint8_t sq = radio->squelch.value;
  IncDec8(&sq, 0, 10, next ? 1 : -1);
  RADIO_SetSquelch(sq);
}

void RADIO_NextFreqNoClicks(bool next) {

  if (radio->channel >= 0) {
    RADIO_NextCH(next);
    return;
  }

  const int8_t dir = next ? 1 : -1;
  const uint32_t step = StepFrequencyTable[radio->step];

  uint32_t f = radio->rxF + step * dir;
  /* if (!PRESET_InRange(f, gCurrentPreset)) {
    if (next) {
      RADIO_TuneTo(gCurrentPreset.rxF);
    } else {
      RADIO_TuneTo(gCurrentPreset.txF - gCurrentPreset.txF % step);
    }
  } else { */
  // f = CHANNELS_GetF(&gCurrentPreset, CHANNELS_GetChannel(&gCurrentPreset,
  // f));
  radio->channel = -1;
  radio->txF = 0;
  radio->rxF = f;
  RADIO_SetupByCurrentVFO();
  // }
  onVfoUpdate();
}

bool RADIO_NextPresetFreqXBandEx(bool next, bool tune, bool precise) {
  uint32_t steps = CHANNELS_GetSteps(&gCurrentPreset);
  int64_t step = CHANNELS_GetChannel(&gCurrentPreset, radio->rxF);
  bool switchBand = false;

  if (next) {
    step++;
  } else {
    step--;
  }

  if (step < 0) {
    // get previous preset
    switchBand = true;
    if (gSettings.crossBandScan) {
      PRESETS_SelectPresetRelativeByScanlist(false);
    }
    steps = CHANNELS_GetSteps(&gCurrentPreset);
    step = steps - 1;
  } else if (step >= steps) {
    // get next preset
    switchBand = true;
    if (gSettings.crossBandScan) {
      PRESETS_SelectPresetRelativeByScanlist(true);
    }
    step = 0;
  }
  radio->rxF = CHANNELS_GetF(&gCurrentPreset, step);
  if (tune) {
    RADIO_TuneToPure(radio->rxF, precise);
  }
  return switchBand;
}

void RADIO_NextPresetFreqXBand(bool next) {
  RADIO_NextPresetFreqXBandEx(next, true, true);
}

void RADIO_ToggleModulation(void) {
  radio->modulation = getNextModulation();

  // NOTE: for right BW after switching from WFM to another
  RADIO_SetupBandParams();
  onVfoUpdate();
}

void RADIO_UpdateStep(bool inc) {
  uint8_t step = radio->step;
  IncDec8(&step, 0, STEP_500_0kHz, inc ? 1 : -1);
  radio->step = step;
  onVfoUpdate();
}

void RADIO_ToggleListeningBW(void) {
  if (radio->bw == BK4819_FILTER_BW_26k) {
    radio->bw = BK4819_FILTER_BW_6k;
  } else {
    ++radio->bw;
  }

  RADIO_SetFilterBandwidth(radio->bw);

  onVfoUpdate();
}

void RADIO_ToggleTxPower(void) {
  if (radio->power == TX_POW_HIGH) {
    radio->power = TX_POW_ULOW;
  } else {
    ++radio->power;
  }

  onVfoUpdate();
}

bool RADIO_HasSi() { return BK1080_ReadRegister(1) != 0x1080; }

void RADIO_SendDTMF(const char *pattern, ...) {
  char str[32] = {0};
  va_list args;
  va_start(args, pattern);
  vsnprintf(str, 31, pattern, args);
  va_end(args);
  RADIO_ToggleTX(true);
  if (gTxState == TX_ON) {
    SYSTEM_DelayMs(200);
    BK4819_EnterDTMF_TX(true);
    BK4819_PlayDTMFString(str, true, 100, 100, 100, 100);
    RADIO_ToggleTX(false);
  }
}
