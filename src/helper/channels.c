#include "channels.h"
#include "../driver/eeprom.h"
#include "../driver/uart.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include <stddef.h>
#include <string.h>

Preset gCurrentPreset;

int16_t gScanlistSize = 0;
uint16_t gScanlist[SCANLIST_MAX] = {0};
CHType gScanlistType = TYPE_CH;

static int8_t presetlistSize = 0;
static uint16_t presetChannel[PRESETS_COUNT_MAX] = {0};

// to use instead of predefined when we need to keep step, etc
Preset defaultPreset = {
    .name = "default",
    .step = STEP_25_0kHz,
    .bw = BK4819_FILTER_BW_12k,
    .squelch =
        {
            .type = SQUELCH_RSSI_NOISE_GLITCH,
            .value = 4,
        },
    .gainIndex = 18,
    .rxF = 0,
    .txF = 130000000,
};

Preset defaultPresets[PRESETS_COUNT_MAX] = {
    // si4732 presets
    (Preset){.name="LW",.rxF=15300,.txF=27900,.modulation=MOD_AM,.step=STEP_9_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=15300,.gainIndex=27},
    (Preset){.name="MW",.rxF=52200,.txF=170100,.modulation=MOD_AM,.step=STEP_9_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=52200,.gainIndex=27},
    (Preset){.name="120m",.rxF=230000,.txF=249500,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=230000,.gainIndex=27},
    (Preset){.name="90m",.rxF=320000,.txF=340000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=320000,.gainIndex=27},
    (Preset){.name="75m",.rxF=390000,.txF=400000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=390000,.gainIndex=27},
    (Preset){.name="60m",.rxF=475000,.txF=506000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=475000,.gainIndex=27},
    (Preset){.name="49m",.rxF=585000,.txF=635000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=585000,.gainIndex=27},
    (Preset){.name="41m",.rxF=720000,.txF=750000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=720000,.gainIndex=27},
    (Preset){.name="31m",.rxF=940000,.txF=999000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=940000,.gainIndex=27},
    (Preset){.name="25m",.rxF=1160000,.txF=1210000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=1160000,.gainIndex=27},
    (Preset){.name="22m",.rxF=1350000,.txF=1387000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=1350000,.gainIndex=27},
    (Preset){.name="19m",.rxF=1510000,.txF=1560000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=1510000,.gainIndex=27},
    (Preset){.name="16m",.rxF=1755000,.txF=1805000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=1755000,.gainIndex=27},
    (Preset){.name="15m",.rxF=1890000,.txF=1902000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=1890000,.gainIndex=27},
    (Preset){.name="13m",.rxF=2145000,.txF=2185000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=2145000,.gainIndex=27},
    (Preset){.name="11m",.rxF=2560000,.txF=2610000,.modulation=MOD_AM,.step=STEP_5_0kHz,.bw=BK4819_FILTER_BW_14k,.misc.lastUsedFreq=2560000,.gainIndex=27},
    (Preset){
        .rxF = 181000,
        .txF = 200000,
        .name = "160m HAM",
        .step = STEP_1_0kHz,
        .modulation = MOD_LSB,
        .bw = BK4819_FILTER_BW_12k,
        .misc = {
            .lastUsedFreq = 181000,
        },
    },
    (Preset){
        .rxF = 350000,
        .txF = 380000,
        .name = "80m HAM",
        .step = STEP_1_0kHz,
        .modulation = MOD_LSB,
        .bw = BK4819_FILTER_BW_12k,
        .misc = {
            .lastUsedFreq = 364800,
        },
    },
    (Preset){
        .rxF = 700000,
        .txF = 719999,
        .name = "40m HAM",
        .step = STEP_1_0kHz,
        .modulation = MOD_LSB,
        .bw = BK4819_FILTER_BW_12k,
        .misc = {
            .lastUsedFreq = 710000,
        },
    },
    (Preset){
        .rxF = 1010000,
        .txF = 1015000,
        .name = "30m HAM",
        .step = STEP_1_0kHz,
        .modulation = MOD_LSB,
        .bw = BK4819_FILTER_BW_12k,
        .misc = {
            .lastUsedFreq = 1010000,
        },
    },
    (Preset){
        .rxF = 1400000,
        .txF = 1435000,
        .name = "20m HAM",
        .step = STEP_1_0kHz,
        .modulation = MOD_LSB,
        .bw = BK4819_FILTER_BW_12k,
        .misc = {
            .lastUsedFreq = 1400000,
        },
    },
    (Preset){
        .rxF = 1806800,
        .txF = 1816800,
        .name = "17m HAM",
        .step = STEP_1_0kHz,
        .modulation = MOD_USB,
        .bw = BK4819_FILTER_BW_12k,
        .misc = {
            .lastUsedFreq = 1812000,
        },
    },
    (Preset){
        .rxF = 2100000,
        .txF = 2145000,
        .name = "15m HAM",
        .step = STEP_1_0kHz,
        .modulation = MOD_USB,
        .bw = BK4819_FILTER_BW_12k,
        .misc = {
            .lastUsedFreq = 2115100,
        },
    },
    (Preset){
        .rxF = 2489000,
        .txF = 2499000,
        .name = "12m HAM",
        .step = STEP_1_0kHz,
        .modulation = MOD_USB,
        .bw = BK4819_FILTER_BW_12k,
        .misc = {
            .lastUsedFreq = 2494000,
        },
    },
    (Preset){
        .rxF = 2800000,
        .txF = 2969999,
        .name = "10m HAM",
        .step = STEP_1_0kHz,
        .modulation = MOD_USB,
        .bw = BK4819_FILTER_BW_12k,
        .misc = {
            .lastUsedFreq = 2822500,
        },
    },
    (Preset){
        .rxF = 1500000,
        .txF = 2696499,
        .name = "Military1",
        .step = STEP_5_0kHz,
        .modulation = MOD_AM,
        .bw = BK4819_FILTER_BW_9k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 1500000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 2696500,
        .txF = 2760124,
        .name = "CB EU",
        .step = STEP_10_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_9k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 2696500,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 2760125,
        .txF = 2799125,
        .name = "CB UK",
        .step = STEP_10_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_9k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 2760125,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 2799130,
        .txF = 6399999,
        .name = "Military2",
        .step = STEP_12_5kHz,
        .modulation = MOD_AM,
        .bw = BK4819_FILTER_BW_9k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 2799130,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 6400000,
        .txF = 8799999,
        .name = "Military3",
        .step = STEP_12_5kHz,
        .modulation = MOD_AM,
        .bw = BK4819_FILTER_BW_9k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 7100000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 8800000,
        .txF = 10799999,
        .name = "Bcast FM",
        .step = STEP_100_0kHz,
        .modulation = MOD_WFM,
        .bw = BK4819_FILTER_BW_9k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 10320000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 10800000,
        .txF = 11799999,
        .name = "108-118",
        .step = STEP_12_5kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 10800000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 11800000,
        .txF = 13699999,
        .name = "Air",
        .step = STEP_12_5kHz,
        .modulation = MOD_AM,
        .bw = BK4819_FILTER_BW_9k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 13170000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 13700000,
        .txF = 14399999,
        .name = "Business1",
        .step = STEP_12_5kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_9k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 13510000,
            .powCalib = {38, 67, 130},
        },
    },
    (Preset){
        .rxF = 14400000,
        .txF = 14599999,
        .name = "2m HAM",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 14550000,
            .powCalib = {38, 63, 138},
        },
    },
    (Preset){
        .rxF = 14600000,
        .txF = 14799999,
        .name = "146-148",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 14600000,
            .powCalib = {38, 63, 138},
        },
    },
    (Preset){
        .rxF = 14800000,
        .txF = 15549999,
        .name = "Business2",
        .step = STEP_12_5kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 14810000,
            .powCalib = {37, 60, 130},
        },
        .radio = RADIO_BK4819,
    },
    (Preset){
        .rxF = 15550000,
        .txF = 16199999,
        .name = "Marine",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 15650000,
            .powCalib = {37, 60, 130},
        },
        .radio = RADIO_BK4819,
    },
    (Preset){
        .rxF = 16200000,
        .txF = 17399999,
        .name = "Business3",
        .step = STEP_12_5kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 16300000,
            .powCalib = {37, 60, 130},
        },
        .radio = RADIO_BK4819,
    },
    (Preset){
        .rxF = 17400000,
        .txF = 24499999,
        .name = "MSatcom1",
        .step = STEP_25_0kHz,
        .modulation = MOD_AM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 20575000,
            .powCalib = {46, 55, 140},
        },
    },
    (Preset){
        .rxF = 24500000,
        .txF = 26999999,
        .name = "MSatcom2",
        .step = STEP_25_0kHz,
        .modulation = MOD_AM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 25355000,
            .powCalib = {58, 80, 140},
        },
    },
    (Preset){
        .rxF = 27000000,
        .txF = 42999999,
        .name = "Military4",
        .step = STEP_12_5kHz,
        .modulation = MOD_AM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 33605000,
            .powCalib = {77, 95, 140},
        },
    },
    (Preset){
        .rxF = 43000000,
        .txF = 43307499,
        .name = "70cmHAM1",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 43230000,
            .powCalib = {40, 65, 140},
        },
    },
    (Preset){
        .rxF = 43307500,
        .txF = 43477500,
        .name = "LPD",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = true,
        .misc = {
            .lastUsedFreq = 43325000,
            .powCalib = {40, 65, 140},
        },
    },
    (Preset){
        .rxF = 43480000,
        .txF = 43999999,
        .name = "70cmHAM2",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 43480000,
            .powCalib = {40, 65, 140},
        },
    },

    (Preset){
        .rxF = 44000000,
        .txF = 44600624,
        .name = "Business4",
        .step = STEP_12_5kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 44000000,
            .powCalib = {40, 65, 140},
        },
    },
    (Preset){
        .rxF = 44600625,
        .txF = 44619375,
        .name = "PMR",
        .step = STEP_6_25kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = true,
        .misc = {
            .lastUsedFreq = 44609375,
            .powCalib = {40, 65, 140},
        },
    },
    (Preset){
        .rxF = 44620000,
        .txF = 46256249,
        .name = "Business5",
        .step = STEP_12_5kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 44620000,
            .powCalib = {40, 65, 140},
        },
    },
    (Preset){
        .rxF = 46256250,
        .txF = 46273749,
        .name = "FRS/G462",
        .step = STEP_12_5kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 46256250,
            .powCalib = {40, 65, 140},
        },
    },
    (Preset){
        .rxF = 46273750,
        .txF = 46756249,
        .name = "Business6",
        .step = STEP_12_5kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 46302500,
            .powCalib = {40, 65, 140},
        },
    },
    (Preset){
        .rxF = 46756250,
        .txF = 46774999,
        .name = "FRS/G467",
        .step = STEP_12_5kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 46756250,
            .powCalib = {40, 65, 140},
        },
    },
    (Preset){
        .rxF = 46775000,
        .txF = 46999999,
        .name = "Business7",
        .step = STEP_12_5kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 46980000,
            .powCalib = {40, 64, 140},
        },
    },
    (Preset){
        .rxF = 47000000,
        .txF = 61999999,
        .name = "470-620",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 50975000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 84000000,
        .txF = 86299999,
        .name = "840-863",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 85500000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 86300000,
        .txF = 86999999,
        .name = "LORA",
        .step = STEP_25_0kHz, // 125 actually
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 86400000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 87000000,
        .txF = 88999999,
        .name = "870-890",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 87000000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 89000000,
        .txF = 95999999,
        .name = "GSM-900",
        .step = STEP_100_0kHz, // 200 actually
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 89000000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 96000000,
        .txF = 125999999,
        .name = "960-1260",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 96000000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 126000000,
        .txF = 129999999,
        .name = "23cm HAM",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 129750000,
            .powCalib = {50, 100, 140},
        },
    },
    (Preset){
        .rxF = 130000000,
        .txF = 134000000,
        .name = "1.3-1.34",
        .step = STEP_25_0kHz,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_12k,
        .allowTx = false,
        .misc = {
            .lastUsedFreq = 130000000,
            .powCalib = {50, 100, 140},
        },
    },
};
// char (*__defpres)[sizeof(defaultPresets)/sizeof(Preset)] = 1;

static uint32_t getChannelsEnd() {
  uint32_t eepromSize = SETTINGS_GetEEPROMSize();
  uint32_t minSizeWithPatch = CHANNELS_OFFSET + CH_SIZE + PATCH_SIZE;
  if (eepromSize < minSizeWithPatch) {
    return eepromSize;
  }
  return eepromSize - PATCH_SIZE;
}

static uint32_t GetChannelOffset(int16_t num) {
  return CHANNELS_OFFSET + num * CH_SIZE;
}

uint16_t CHANNELS_GetCountMax(void) {
  uint16_t n = (getChannelsEnd() - CHANNELS_OFFSET) / CH_SIZE;
  return n < SCANLIST_MAX ? n : SCANLIST_MAX;
}

void CHANNELS_Load(int16_t num, CH *p) {
  if (num >= 0) {
    EEPROM_ReadBuffer(GetChannelOffset(num), p, CH_SIZE);
    Log(">> R CH%u '%s': f=%u, radio=%u", num, p->name, p->rxF, p->radio);
  }
}

void CHANNELS_Save(int16_t num, CH *p) {
  if (num >= 0) {
    Log(">> W CH%u '%s': f=%u, radio=%u", num, p->name, p->rxF, p->radio);
    EEPROM_WriteBuffer(GetChannelOffset(num), p, CH_SIZE);
  }
}

void CHANNELS_Delete(int16_t num) {
  CH _ch;
  memset(&_ch, 0, sizeof(_ch));
  CHANNELS_Save(num, &_ch);
}

bool CHANNELS_Existing(int16_t num) {
  if (num < 0 || num >= CHANNELS_GetCountMax()) {
    return false;
  }
  return CHANNELS_GetMeta(num).type != TYPE_EMPTY;
}

uint8_t CHANNELS_Scanlists(int16_t num) {
  uint8_t scanlists;
  uint32_t addr = GetChannelOffset(num) + offsetof(CH, scanlists);
  EEPROM_ReadBuffer(addr, &scanlists, 1);
  return scanlists;
}

int16_t CHANNELS_Next(int16_t base, bool next) {
  int16_t si = base;
  int16_t max = CHANNELS_GetCountMax() - 2 - PRESETS_COUNT_MAX;
  IncDecI16(&si, 0, max, next ? 1 : -1);
  int16_t i = si;
  if (next) {
    for (; i < max; ++i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
    for (i = 0; i < base; ++i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
  } else {
    for (; i >= 0; --i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
    for (i = max - 1; i > base; --i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
  }
  return -1;
}

void CHANNELS_LoadScanlist(CHType type, uint8_t n) {
  gSettings.currentScanlist = n;
  uint8_t scanlistMask = 1 << n;
  gScanlistSize = 0;
  for (int16_t i = 0; i < CHANNELS_GetCountMax(); ++i) {
    if (type != TYPE_ALL && CHANNELS_GetMeta(i).type != type) {
      continue;
    }
    if (n == 15 || (CHANNELS_Scanlists(i) & scanlistMask) == scanlistMask) {
      gScanlist[gScanlistSize] = i;
      // Log("gScanlist[%u]=%u", gScanlistSize, i);
      gScanlistSize++;
    }
  }
  SETTINGS_Save();
}

void CHANNELS_LoadBlacklistToLoot() {
  uint8_t scanlistMask = 1 << 7;
  for (int16_t i = 0; i < CHANNELS_GetCountMax(); ++i) {
    if (!CHANNELS_Existing(i)) {
      continue;
    }
    if ((CHANNELS_Scanlists(i) & scanlistMask) == scanlistMask) {
      CH ch;
      CHANNELS_Load(i, &ch);
      Loot *loot = LOOT_AddEx(ch.rxF, true);
      loot->open = false;
      loot->blacklist = true;
      loot->lastTimeOpen = 0;
    }
  }
}

uint16_t CHANNELS_GetStepSize(CH *p) { return StepFrequencyTable[p->step]; }

uint32_t CHANNELS_GetSteps(CH *p) {
  return (p->txF - p->rxF) / CHANNELS_GetStepSize(p) + 1;
}

uint32_t CHANNELS_GetF(CH *p, uint32_t channel) {
  return p->rxF + channel * CHANNELS_GetStepSize(p);
}

uint32_t CHANNELS_GetChannel(CH *p, uint32_t f) {
  return (f - p->rxF) / CHANNELS_GetStepSize(p);
}

CHMeta CHANNELS_GetMeta(int16_t num) {
  CHMeta meta;
  EEPROM_ReadBuffer(GetChannelOffset(num) + offsetof(CH, meta), &meta, 1);
  return meta;
}

// --- PRESETS ---

int8_t PRESETS_Size(void) { return presetlistSize < 0 ? 0 : presetlistSize; }

Preset PRESETS_Item(int8_t i) { return defaultPresets[i]; }

bool PRESET_InRange(const uint32_t f, const Preset p) {
  return f >= p.rxF && f <= p.txF;
}

int8_t PRESET_IndexOf(Preset p) {
  for (uint8_t i = 0; i < PRESETS_Size(); ++i) {
    Preset tmp = PRESETS_Item(i);
    if (memcmp(&tmp, &p, sizeof(p)) == 0) {
      return i;
    }
  }
  return -1;
}

Preset PRESET_ByFrequency(uint32_t f) {
  uint32_t smallerBW = BK4819_F_MAX;
  int16_t index = -1;
  for (uint8_t i = 0; i < PRESETS_Size(); ++i) {
    Preset item = PRESETS_Item(i);
    if (PRESET_InRange(f, item)) {
      uint32_t bw = item.txF - item.rxF;
      if (bw < smallerBW) {
        smallerBW = bw;
        index = i;
      }
    }
  }
  if (index >= 0) {
    return PRESETS_Item(index);
  }
  return defaultPreset;
}

void PRESET_Select(int8_t i) {
  gCurrentPreset = PRESETS_Item(i);
  gSettings.activePreset = i;
  Log("[i] PRST Select %u", i);
}

void PRESETS_SelectPresetRelative(bool next) {
  int8_t presetIndex = gSettings.activePreset;
  IncDecI8(&presetIndex, 0, PRESETS_Size(), next ? 1 : -1);
  gSettings.activePreset = presetIndex;
  gCurrentPreset = PRESETS_Item(gSettings.activePreset);
  radio->rxF = gCurrentPreset.rxF;
  SETTINGS_DelayedSave();
}

int8_t PRESET_SelectByFrequency(uint32_t f) {
  gCurrentPreset = PRESET_ByFrequency(f);
  int8_t i = PRESET_IndexOf(gCurrentPreset);
  if (i >= 0) {
    gSettings.activePreset = i;
  }
  return i;
}

void PRESETS_SelectPresetRelativeByScanlist(bool next) {
  uint8_t index = gSettings.activePreset;
  uint8_t sl = gSettings.currentScanlist;
  uint8_t scanlistMask = 1 << sl;
  PRESETS_SelectPresetRelative(next);
  while (gSettings.activePreset != index) {
    if (sl == 15 || (gCurrentPreset.scanlists & scanlistMask) == scanlistMask) {
      return;
    }
    PRESETS_SelectPresetRelative(next);
  }
}

bool PRESETS_Load(void) {
  for (int16_t chNum = CHANNELS_GetCountMax() - 2; chNum >= 0; --chNum) {
    if (CHANNELS_GetMeta(chNum).type != TYPE_PRESET) {
      continue;
    }

    CHANNELS_Load(chNum, &defaultPresets[presetlistSize]);

    presetChannel[presetlistSize] = chNum;

    presetlistSize++;

    if (presetlistSize >= PRESETS_COUNT_MAX) {
      break;
    }
  }
  return true;
}

void PRESETS_SaveCurrent(void) {
  CHANNELS_Save(presetChannel[gSettings.activePreset], &gCurrentPreset);
}
