#include "presetlist.h"
#include "../driver/eeprom.h"
#include "../helper/measurements.h"
#include "../settings.h"

Preset *gCurrentPreset;
static Preset presets[PRESETS_COUNT] = {0};
static int8_t loadedCount = 0;

// to use instead of predefined when we need to keep step, etc
Preset defaultPreset = {
    .band =
        (Band){
            .name = "default",
            .step = STEP_25_0kHz,
            .bw = BK4819_FILTER_BW_WIDE,
            .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            .squelch = 3,
            .gainIndex = 18,
        },
};

Preset
    defaultPresets[PRESETS_COUNT] =
        {
            (Preset){
                .band =
                    {
                        .bounds = {1500000, 2696499},
                        .name = "Military1",
                        .step = STEP_5_0kHz,
                        .modulation = MOD_AM,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .allowTx = false,
                .lastUsedFreq = 1500000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {2696500, 2760124},
                        .name = "CB EU",
                        .step = STEP_10_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .allowTx = false,
                .lastUsedFreq = 2696500,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {2760125, 2799125},
                        .name = "CB UK",
                        .step = STEP_10_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .allowTx = false,
                .lastUsedFreq = 2760125,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {2799130, 6399999},
                        .name = "Military2",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_AM,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .allowTx = false,
                .lastUsedFreq = 2799130,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {6400000, 8799999},
                        .name = "Military3",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_AM,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .allowTx = false,
                .lastUsedFreq = 7100000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {8800000, 10799999},
                        .name = "Bcast FM",
                        .step = STEP_100_0kHz,
                        .modulation = MOD_WFM,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .allowTx = false,
                .lastUsedFreq = 10320000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {10800000, 11799999},
                        .name = "108-118",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 10800000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {11800000, 13499999},
                        .name = "Air",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_AM,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .allowTx = false,
                .lastUsedFreq = 13170000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {13500000, 14399999},
                        .name = "Business1",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .allowTx = false,
                .lastUsedFreq = 13510000,
                .powCalib = {38, 67, 130},
            },
            (Preset){
                .band =
                    {
                        .bounds = {14400000, 14599999},
                        .name = "2m HAM",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 14550000,
                .powCalib = {38, 63, 138},
            },
            (Preset){
                .band =
                    {
                        .bounds = {14600000, 14799999},
                        .name = "146-148",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 14600000,
                .powCalib = {38, 63, 138},
            },
            (Preset){
                 .band =
                   {
                        .bounds = {14800000, 15549999},
                        .name = "Business2",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 14810000,
                .powCalib = {37, 60, 130},
                .radio = RADIO_BK4819,
            },
            (Preset){
                .band =
                    {
                        .bounds = {15550000, 16199999},
                        .name = "Marine",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 15650000,
                .powCalib = {37, 60, 130},
                .radio = RADIO_BK4819,
            },
            (Preset){
                .band =
                    {
                    .bounds = {16200000, 17399999},
                    .name = "Business3",
                    .step = STEP_12_5kHz,
                    .modulation = MOD_FM,
                    .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 16300000,
                .powCalib = {37, 60, 130},
                .radio = RADIO_BK4819,
            },
            (Preset){
                .band =
                    {
                        .bounds = {17400000, 24499999},
                        .name = "MSatcom1",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_AM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 20575000,
                .powCalib = {46, 55, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {24500000, 26999999},
                        .name = "MSatcom2",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_AM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 25355000,
                .powCalib = {58, 80, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {27000000, 42999999},
                        .name = "Military4",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_AM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 33605000,
                .powCalib = {77, 95, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {43000000, 43307499},
                        .name = "70cmHAM1",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 43230000,
                .powCalib = {40, 65, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {43307500, 43477500},
                        .name = "LPD",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = true,
                .lastUsedFreq = 43325000,
                .powCalib = {40, 65, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {43480000, 43999999},
                        .name = "70cmHAM2",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 43480000,
                .powCalib = {40, 65, 140},
            },

            (Preset){
                .band =
                    {
                        .bounds = {44000000, 44600624},
                        .name = "Business4",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 44000000,
                .powCalib = {40, 65, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {44600625, 44619375},
                        .name = "PMR",
                        .step = STEP_6_25kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = true,
                .lastUsedFreq = 44609375,
                .powCalib = {40, 65, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {44620000, 46256249},
                        .name = "Business5",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 44620000,
                .powCalib = {40, 65, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {46256250, 46273749},
                        .name = "FRS/G462",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 46256250,
                .powCalib = {40, 65, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {46273750, 46756249},
                        .name = "Business6",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 46302500,
                .powCalib = {40, 65, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {46756250, 46774999},
                        .name = "FRS/G467",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 46756250,
                .powCalib = {40, 65, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {46775000, 46999999},
                        .name = "Business7",
                        .step = STEP_12_5kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 46980000,
                .powCalib = {40, 64, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {47000000, 61999999},
                        .name = "470-620",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 50975000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {84000000, 86299999},
                        .name = "840-863",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 85500000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {86300000, 86999999},
                        .name = "LORA",
                        .step = STEP_25_0kHz, // 125 actually
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 86400000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {87000000, 88999999},
                        .name = "870-890",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 87000000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {89000000, 95999999},
                        .name = "GSM-900",
                        .step = STEP_100_0kHz, // 200 actually
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 89000000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {96000000, 125999999},
                        .name = "960-1260",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 96000000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {126000000, 129999999},
                        .name = "23cm HAM",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 129750000,
                .powCalib = {50, 100, 140},
            },
            (Preset){
                .band =
                    {
                        .bounds = {130000000, 134000000},
                        .name = "1.3-1.34",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                    },
                .allowTx = false,
                .lastUsedFreq = 130000000,
                .powCalib = {50, 100, 140},
            },

            // si4732 presets
            (Preset){
                .band =
                    {
                        .bounds = {181000, 200000},
                        .name = "160m HAM",
                        .step = STEP_1_0kHz,
                        .modulation = MOD_LSB,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .lastUsedFreq = 181000,
            },
            (Preset){
                .band =
                    {
                        .bounds = {350000, 380000},
                        .name = "80m HAM",
                        .step = STEP_1_0kHz,
                        .modulation = MOD_LSB,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .lastUsedFreq = 364800,
            },
            (Preset){
                .band =
                    {
                        .bounds = {700000, 719999},
                        .name = "40m HAM",
                        .step = STEP_1_0kHz,
                        .modulation = MOD_LSB,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .lastUsedFreq = 710000,
            },
            (Preset){
                .band =
                    {
                        .bounds = {1400000, 1435000},
                        .name = "20m HAM",
                        .step = STEP_1_0kHz,
                        .modulation = MOD_USB,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .lastUsedFreq = 1400000,
            },
            (Preset){
                .band =
                    {
                        .bounds = {2800000, 2969999},
                        .name = "10m HAM",
                        .step = STEP_1_0kHz,
                        .modulation = MOD_USB,
                        .bw = BK4819_FILTER_BW_NARROW,
                    },
                .lastUsedFreq = 2800000,
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

Preset *PRESET_ByFrequency(uint32_t f) {
  uint32_t smallerBW = 134000000;
  Preset *p = 0;
  for (uint8_t i = 0; i < PRESETS_Size(); ++i) {
    if (PRESET_InRange(f, &presets[i])) {
      uint32_t bw = presets[i].band.bounds.end - presets[i].band.bounds.start;
      if (bw < smallerBW) {
        smallerBW = bw;
        p = &presets[i];
      }
    }
  }
  if (p) {
    return p;
  }
  return &defaultPreset;
}

int8_t PRESET_SelectByFrequency(uint32_t f) {
  gCurrentPreset = PRESET_ByFrequency(f);
  int8_t i = PRESET_IndexOf(gCurrentPreset);
  if (i >= 0) {
    gSettings.activePreset = i;
  }
  return i;
  /* if (PRESET_InRange(f, gCurrentPreset)) {
    return gSettings.activePreset;
  }
  for (int8_t i = 0; i < PRESETS_Size(); ++i) {
    if (PRESET_InRange(f, &presets[i])) {
      PRESET_Select(i);
      return i;
    }
  }
  gCurrentPreset = &defaultPreset; // TODO: make preset between near bands
  return -1; */
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
