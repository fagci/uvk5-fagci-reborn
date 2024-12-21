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
#include "ui/spectrum.h"

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

static ModulationType MODS_WFM[] = {
    MOD_WFM,
};

static ModulationType MODS_BOTH_PATCH[] = {
    MOD_FM, MOD_AM, MOD_USB, MOD_LSB, MOD_BYP, MOD_RAW, MOD_WFM,
};

static ModulationType MODS_BOTH[] = {
    MOD_FM, MOD_AM, MOD_USB, MOD_BYP, MOD_RAW, MOD_WFM,
};

static ModulationType MODS_SI4732_PATCH[] = {
    MOD_AM,
    MOD_LSB,
    MOD_USB,
};

static ModulationType MODS_SI4732[] = {
    MOD_AM,
};

static void loadVFO(uint8_t num) {
  CHANNELS_Load(CHANNELS_GetCountMax() - 2 + num, &gVFO[num]);
}

static void saveVFO(uint8_t num) {
  CHANNELS_Save(CHANNELS_GetCountMax() - 2 + num, &gVFO[num]);
}

static uint8_t indexOfMod(ModulationType *arr, uint8_t n, ModulationType t) {
  for (uint8_t i = 0; i < n; ++i) {
    if (arr[i] == t) {
      return i;
    }
  }
  return 0;
}

static ModulationType getNextModulation(bool next) {
  uint8_t sz = ARRAY_SIZE(MODS_BK4819);
  ModulationType *items = MODS_BK4819;

  if (radio->rxF <= SI47XX_F_MAX && radio->rxF >= BK4819_F_MIN) {
    if (isPatchPresent) {
      items = MODS_BOTH_PATCH;
      sz = ARRAY_SIZE(MODS_BOTH_PATCH);
    } else {
      items = MODS_BOTH;
      sz = ARRAY_SIZE(MODS_BOTH);
    }
  } else if (radio->rxF <= SI47XX_F_MAX) {
    if (isPatchPresent) {
      items = MODS_SI4732_PATCH;
      sz = ARRAY_SIZE(MODS_SI4732_PATCH);
    } else {
      items = MODS_SI4732;
      sz = ARRAY_SIZE(MODS_SI4732);
    }
  }

  uint8_t curIndex = indexOfMod(items, sz, radio->modulation);

  if (next) {
    IncDec8(&curIndex, 0, sz, 1);
  }

  return items[curIndex];
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
  TaskRemove(RADIO_SaveCurrentVFO);
  TaskAdd("VFO sav", RADIO_SaveCurrentVFO, 2000, false, 0);
}

static void onBandUpdate(void) {
  TaskRemove(BANDS_SaveCurrent);
  TaskAdd("PRS sav", BANDS_SaveCurrent, 2000, false, 0);
}

static void setupToneDetection() {
  uint16_t InterruptMask =
      BK4819_REG_3F_CxCSS_TAIL | BK4819_REG_3F_DTMF_5TONE_FOUND;
  if (gSettings.dtmfdecode) {
    BK4819_EnableDTMF();
  } else {
    BK4819_DisableDTMF();
  }
  switch (radio->code.rx.type) {
  case CODE_TYPE_DIGITAL:
  case CODE_TYPE_REVERSE_DIGITAL:
    // Log("DCS on");
    BK4819_SetCDCSSCodeWord(
        DCS_GetGolayCodeWord(radio->code.rx.type, radio->code.rx.value));
    InterruptMask |= BK4819_REG_3F_CDCSS_FOUND | BK4819_REG_3F_CDCSS_LOST;
    break;
  case CODE_TYPE_CONTINUOUS_TONE:
    // Log("CTCSS on");
    BK4819_SetCTCSSFrequency(CTCSS_Options[radio->code.rx.value]);
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
  SQL sq = GetSql(radio->squelch.value);

  // TODO: automatic sql when msmTime < 10

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

static uint8_t calculateOutputPower(Band *p) {
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

static void setupBandplanParams() {}

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

uint32_t RADIO_GetTXFEx(VFO *vfo, Band *p) {
  uint32_t txF = vfo->rxF;

  if (vfo->txF && p->offsetDir == OFFSET_FREQ) {
    txF = vfo->txF;
  } else if (p->txF && p->offsetDir != OFFSET_NONE) {
    txF = vfo->rxF + (p->offsetDir == OFFSET_PLUS ? p->txF : -p->txF);
  }

  return txF;
}

uint32_t RADIO_GetTXF(void) { return RADIO_GetTXFEx(radio, &gCurrentBand); }

TXState RADIO_GetTXState(uint32_t txF) {
  if (gSettings.upconverter) {
    return TX_DISABLED_UPCONVERTER;
  }

  const Band txBand = BAND_ByFrequency(txF);

  if (!txBand.allowTx || RADIO_GetRadio() != RADIO_BK4819 ||
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
  Band txBand = BAND_ByFrequency(txF);
  return Clamp(calculateOutputPower(&txBand), 0, 0x91);
}

void RADIO_ToggleTX(bool on) {
  uint32_t txF = RADIO_GetTXF();
  uint8_t power = RADIO_GetTxPower(txF);
  SVC_Toggle(SVC_FC, false, 0);
  RADIO_ToggleTXEX(on, txF, power, true);
}

bool RADIO_IsChMode() { return radio->channel >= 0; }

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
  radio->modulation = getNextModulation(false);
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
  BAND_SelectByFrequency(radio->rxF);

  RADIO_SwitchRadio();
  RADIO_SetupBandParams();
  RADIO_TuneToPure(radio->rxF, !gMonitorMode);
}

// USE CASE: set vfo temporary for current app
void RADIO_TuneTo(uint32_t f) {
  if (RADIO_IsChMode()) {
    radio->channel = -1;
    snprintf(radio->name, 5, "VFO%u", gSettings.activeVFO + 1);
  }
  radio->txF = 0;
  radio->rxF = f;
  RADIO_SetupByCurrentVFO();
}

// USE CASE: set vfo and use in another app
void RADIO_TuneToSave(uint32_t f) {
  RADIO_TuneTo(f);
  gCurrentBand.misc.lastUsedFreq = f;
  RADIO_SaveCurrentVFO();
  BANDS_SaveCurrent();
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

void RADIO_LoadCurrentVFO(void) {
  for (uint8_t i = 0; i < 2; ++i) {
    loadVFO(i);
    // Log("gVFO(%u)= (f=%u, radio=%u)", i + 1, gVFO[i].rxF, gVFO[i].radio);
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
    SI47XX_SetAutomaticGainControl(disableAGC, disableAGC ? gainIndex : 0);
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
  // Log("RADIO_SetupBandParams");
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
    setupToneDetection();
    break;
  case RADIO_BK1080:
    break;
  case RADIO_SI4732:
    if (mod == MOD_FM) {
      SI47XX_SetSeekFmLimits(gCurrentBand.rxF, gCurrentBand.txF);
      SI47XX_SetSeekFmSpacing(StepFrequencyTable[gCurrentBand.step]);
    } else if (mod == MOD_AM) {
      SI47XX_SetSeekAmLimits(gCurrentBand.rxF, gCurrentBand.txF);
      SI47XX_SetSeekAmSpacing(StepFrequencyTable[gCurrentBand.step]);
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

void RADIO_TuneToBand(int16_t num) {
  CH ch;
  CHANNELS_Load(num, &ch);
  radio->fixedBoundsMode = true;
  RADIO_SaveCurrentVFO();
  radio->bw = ch.bw;
  radio->step = ch.step;
  radio->gainIndex = ch.gainIndex;
  radio->modulation = ch.modulation;
  if (BAND_InRange(ch.misc.lastUsedFreq, gCurrentBand)) {
    RADIO_TuneToSave(ch.misc.lastUsedFreq);
  } else {
    RADIO_TuneToSave(ch.rxF);
  }
}

void RADIO_TuneToCH(int16_t num) {
  radio->channel = num;
  RADIO_VfoLoadCH(gSettings.activeVFO);
  RADIO_SaveCurrentVFO();
  RADIO_SetupByCurrentVFO();
}

bool RADIO_TuneToMR(int16_t num) {
  if (CHANNELS_Existing(num)) {
    switch (CHANNELS_GetMeta(num).type) {
    case TYPE_CH:
      RADIO_TuneToCH(num);
      return true;
    case TYPE_BAND:
      RADIO_TuneToBand(num);
      break;
    default:
      break;
    }
  }
  radio->channel = -1;
  return false;
}

// TODO: rewrite into channels.c
void RADIO_NextBand(bool next) {
  RADIO_TuneToBand(CHANNELS_Next(gScanlist[gSettings.activeBand], next));
  SETTINGS_DelayedSave();
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
  if (RADIO_IsChMode()) {
    loadVFO(gSettings.activeVFO);
    radio->channel += 1; // 0 -> 1
    radio->channel *= -1;
    saveVFO(gSettings.activeVFO);
    RADIO_SetupByCurrentVFO();
  } else {
    radio->channel *= -1;
    radio->channel -= 1; // 1 -> 0
    if (CHANNELS_Existing(radio->channel)) {
      RADIO_TuneToMR(radio->channel);
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
  if (RADIO_IsChMode()) {
    RADIO_NextCH(next);
    return;
  }

  const int8_t dir = next ? 1 : -1;
  const uint32_t step = StepFrequencyTable[radio->step];

  uint32_t f = radio->rxF + step * dir;
  if (radio->fixedBoundsMode && !BAND_InRange(f, gCurrentBand)) {
    if (next) {
      f = gCurrentBand.rxF;
    } else {
      f = gCurrentBand.txF - gCurrentBand.txF % step;
    }
  }
  // f = CHANNELS_GetF(&gCurrentBand, CHANNELS_GetChannel(&gCurrentBand,
  // f));
  radio->channel = -1;
  radio->txF = 0;
  radio->rxF = f;
  RADIO_SetupByCurrentVFO();
  // }
  onVfoUpdate();
}

bool RADIO_NextBandFreqXBandEx(bool next, bool precise) {
  bool switchBand = false;
  if (radio->fixedBoundsMode) {
    uint32_t steps = CHANNELS_GetSteps(&gCurrentBand);
    int64_t step = CHANNELS_GetChannel(&gCurrentBand, radio->rxF);

    if (next) {
      step++;
    } else {
      step--;
    }

    if (step < 0) {
      // get previous band
      switchBand = true;
      BANDS_SelectBandRelativeByScanlist(false);
      steps = CHANNELS_GetSteps(&gCurrentBand);
      step = steps - 1;
    } else if (step >= steps) {
      // get next band
      switchBand = true;
      BANDS_SelectBandRelativeByScanlist(true);
      step = 0;
    }
    radio->rxF = CHANNELS_GetF(&gCurrentBand, step);
  } else {
    if (next) {
      radio->rxF += StepFrequencyTable[radio->step];
    } else {
      radio->rxF -= StepFrequencyTable[radio->step];
    }
  }
  // NOTE: was RADIO_TuneToPure,
  // but unbound scan will not change current band & radio
  uint8_t bi = gSettings.activeBand;
  BAND_SelectByFrequency(radio->rxF);
  if (bi != gSettings.activeBand) {
    SP_Init(&gCurrentBand);
  }

  RADIO_SwitchRadio();
  RADIO_SetupBandParams();
  RADIO_TuneToPure(radio->rxF, precise);
  return switchBand;
}

void RADIO_NextBandFreqXBand(bool next) {
  RADIO_NextBandFreqXBandEx(next, true);
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

void RADIO_ToggleModulation(void) {
  if (radio->modulation == getNextModulation(true))
    return;
  radio->modulation = getNextModulation(true);

  // NOTE: for right BW after switching from WFM to another
  RADIO_SetupBandParams();
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
