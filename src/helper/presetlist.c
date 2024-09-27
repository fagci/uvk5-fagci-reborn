#include "presetlist.h"
#include "../driver/eeprom.h"
#include "../helper/measurements.h"
#include "../settings.h"

Preset *gCurrentPreset;
static Preset presets[PRESETS_SIZE_MAX] = {0};
static int8_t loadedCount = 0;

// to use instead of predefined when we need to keep step, etc
Preset defaultPreset = {
    .band =
        (Band){
            .name = "default",
            .step = STEP_25_0kHz,
            .bw = BK4819_FILTER_BW_WIDE,
            .gainIndex = 18,
            .squelch = 3,
            .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
        },
};
//static const PowerCalibration POWER_CALIB_STD = {0x8C, 0x8C, 0x8C};
Preset defaultPresets[33] = {
    (Preset){
        .band =
            {
                .bounds = {1500000, 2799999},
                .name = "15-28",
                .step = STEP_5_0kHz,
                .modulation = MOD_AM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 1500000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {3000000, 6399999},
                .name = "30-64",
                .step = STEP_5_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 3000000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {6400000, 8799999},
                .name = "64-88",
                .step = STEP_100_0kHz,
                .modulation = MOD_WFM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 7100000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {8800000, 10799999},
                .name = "Bcast FM",
                .step = STEP_100_0kHz,
                .modulation = MOD_WFM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 10320000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {10800000, 11799999},
                .name = "108-118",
                .step = STEP_12_5kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 10800000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {11800000, 13499999},
                .name = "Air",
                .step = STEP_12_5kHz,
                .modulation = MOD_AM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 13170000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {13500000, 14399999},
                .name = "135-144",
                .step = STEP_12_5kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 13510000,
                //           38	   67	 130
                .powCalib = {0x26, 0x43, 0x8b},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {14400000, 14799999},
                .name = "2m HAM",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 14550000,
                //           38	   63	 138
                .powCalib = {0x26, 0x3f, 0x8a},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {14800000, 17399999},
                .name = "148-174",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 15300000,
        //                   37    60	 130
                .powCalib = {0x25, 0x3c, 0x82},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {17400000, 24499999},
                .name = "174-245",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 20575000,
        //           46	   55    140
        .powCalib = {0x2e, 0x37, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {24500000, 26999999},
                .name = "Satcom",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 25355000,
        //           58	   80	 130
        .powCalib = {0x3a, 0x50, 0x82},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {27000000, 39999999},
                .name = "270-400",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 33605000,
        //           77	   95    140
        .powCalib = {0x4d, 0x5e, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {40000000, 43307499},
                .name = "400-433",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 42230000,
        //           40	   65	 140
        .powCalib = {0x28, 0x41, 0x8c},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {43307500, 43479999},
                .name = "LPD",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = true,
        .lastUsedFreq = 43325000,
        //           40    65    140
        .powCalib = {0x28, 0x41, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {43480000, 44600624},
                .name = "435-446",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 43700000,
        //           40    65    140
        .powCalib = {0x28, 0x41, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {44600625, 44619375},
                .name = "PMR",
                .step = STEP_6_25kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = true,
        .lastUsedFreq = 44609375,
        //           40    65   140
        .powCalib = {0x28, 0x41, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {44620000, 46256249},
                .name = "446-462",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 46060000,
        //           41	   65	 140
        .powCalib = {0x29, 0x41, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {46256250, 46273749},
                .name = "FRS/G462",
                .step = STEP_12_5kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 46256250,
        //           41	   65	 140
        .powCalib = {0x29, 0x41, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {46273750, 46756249},
                .name = "462-467",
                .step = STEP_12_5kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 46302500,
        //           41	   65	 140
        .powCalib = {0x29, 0x41, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {46756250, 46774999},
                .name = "FRS/G467",
                .step = STEP_12_5kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 46756250,
        //           41	   65	 140
        .powCalib = {0x29, 0x41, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {46775000, 46999999},
                .name = "468-470",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 46980000,
        //           42	   65	 140
        .powCalib = {0x2a, 0x41, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {47000000, 62000000},
                .name = "470-620",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 50975000,
        //           48	   70	 140
        .powCalib = {0x30, 0x46, 0x8c},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {84000000, 86299999},
                .name = "840-863",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 85500000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {86300000, 86999999},
                .name = "LORA",
                .step = STEP_125_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 86400000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {87000000, 88999999},
                .name = "870-890",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 87000000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {89000000, 95999999},
                .name = "GSM-900",
                .step = STEP_200_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 89000000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {96000000, 125999999},
                .name = "960-1260",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 96000000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {126000000, 129999999},
                .name = "23cm HAM",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 129750000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },
    (Preset){
        .band =
            {
                .bounds = {126000000, 134000000},
                .name = "1.3-1.34",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 18,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
        .lastUsedFreq = 126000000,
        //           50    100   140
        .powCalib = {0x32, 0x64, 0x8C},
        .radio = RADIO_BK4819,
    },

    // si4732 presets
    (Preset){
        .band =
            {
                .bounds = {350000, 380000},
                .name = "80m HAM",
                .step = STEP_1_0kHz,
                .modulation = MOD_LSB,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 18,
            },
        .lastUsedFreq = 364800,
        .radio = RADIO_SI4732,
    },
    (Preset){
        .band =
            {
                .bounds = {700000, 719999},
                .name = "40m HAM",
                .step = STEP_1_0kHz,
                .modulation = MOD_LSB,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 18,
            },
        .lastUsedFreq = 710000,
        .radio = RADIO_SI4732,
    },
        (Preset){
        .band =
            {
                .bounds = {1400000, 1435000},
                .name = "20m HAM",
                .step = STEP_1_0kHz,
                .modulation = MOD_USB,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 18,
            },
        .lastUsedFreq = 1410000,
        .radio = RADIO_SI4732,
    },
        (Preset){
        .band =
            {
                .bounds = {2800000, 2970000},
                .name = "10m HAM",
                .step = STEP_1_0kHz,
                .modulation = MOD_USB,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 18,
            },
        .lastUsedFreq = 2810000,
        .radio = RADIO_SI4732,
     },
};
// char (*__defpres)[sizeof(defaultPresets)/sizeof(Preset)] = 1;

void PRESETS_SavePreset(int8_t num, Preset *p) {
  if (num >= 0) {
    EEPROM_WriteBuffer(PRESETS_OFFSET + num * PRESET_SIZE, p, PRESET_SIZE);
  }
}

void PRESETS_LoadPreset(int8_t num, Preset *p) {
  if (num >= 0) {
    EEPROM_ReadBuffer(PRESETS_OFFSET + num * PRESET_SIZE, p, PRESET_SIZE);
  }
}

void PRESETS_SaveCurrent(void) {
  if (gCurrentPreset != &defaultPreset) {
    PRESETS_SavePreset(gSettings.activePreset, gCurrentPreset);
  }
}

int8_t PRESETS_Size(void) { return gSettings.presetsCount; }

Preset *PRESETS_Item(int8_t i) { return &presets[i]; }

void PRESETS_SelectPresetRelative(bool next) {
  int8_t activePreset = gSettings.activePreset;
  IncDecI8(&activePreset, 0, PRESETS_Size(), next ? 1 : -1);
  gSettings.activePreset = activePreset;
  gCurrentPreset = &presets[gSettings.activePreset];
  radio->rx.f = gCurrentPreset->band.bounds.start;
  SETTINGS_DelayedSave();
}

int8_t PRESET_GetCurrentIndex(void) {
  // FIXME: выстроить правильную схему работы с текущим пресетом
  return PRESET_IndexOf(gCurrentPreset);
}

void PRESET_Select(int8_t i) {
  gCurrentPreset = &presets[i];
  gSettings.activePreset = i;
}

bool PRESET_InRange(const uint32_t f, const Preset *p) {
  return f >= p->band.bounds.start && f <= p->band.bounds.end;
}

bool PRESET_InRangeOffset(const uint32_t f, const Preset *p) {
  return f >= p->band.bounds.start + p->offset &&
         f <= p->band.bounds.end + p->offset;
}

int8_t PRESET_IndexOf(Preset *p) {
  for (int8_t i = 0; i < PRESETS_Size(); ++i) {
    if (PRESETS_Item(i) == p) {
      return i;
    }
  }
  return -1;
}

int8_t PRESET_SelectByFrequency(uint32_t f) {
  if (PRESET_InRange(f, gCurrentPreset)) {
    return gSettings.activePreset;
  }
  for (int8_t i = 0; i < PRESETS_Size(); ++i) {
    if (PRESET_InRange(f, &presets[i])) {
      PRESET_Select(i);
      return i;
    }
  }
  gCurrentPreset = &defaultPreset; // TODO: make preset between near bands
  return -1;
}

Preset *PRESET_ByFrequency(uint32_t f) {
  for (uint8_t i = 0; i < PRESETS_Size(); ++i) {
    if (PRESET_InRange(f, &presets[i])) {
      return &presets[i];
    }
  }
  return &defaultPreset;
}

bool PRESETS_Load(void) {
  if (loadedCount < PRESETS_Size()) {
    PRESETS_LoadPreset(loadedCount, &presets[loadedCount]);
    loadedCount++;
    return false;
  }
  return true;
}

uint16_t PRESETS_GetStepSize(Preset *p) {
  return StepFrequencyTable[p->band.step];
}

uint32_t PRESETS_GetSteps(Preset *p) {
  return (p->band.bounds.end - p->band.bounds.start) / PRESETS_GetStepSize(p) +
         1;
}

uint32_t PRESETS_GetF(Preset *p, uint32_t channel) {
  return p->band.bounds.start + channel * PRESETS_GetStepSize(p);
}

uint32_t PRESETS_GetChannel(Preset *p, uint32_t f) {
  return (f - p->band.bounds.start) / PRESETS_GetStepSize(p);
}
