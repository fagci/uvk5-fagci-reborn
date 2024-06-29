#ifndef PRESETLIST_HELPER_H
#define PRESETLIST_HELPER_H

#include "../radio.h"
#include <stdint.h>

#define PRESETS_SIZE_MAX 32

static Preset
    defaultPresets[] =
        {
            (Preset){
                .band =
                    {
                        .bounds = {1500000, 2999999},
                        .name = "15-30",
                        .step = STEP_5_0kHz,
                        .modulation = MOD_AM,
                        .bw = BK4819_FILTER_BW_NARROW,
                        .gainIndex = 18,
                        .squelch = 3,
                        .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
                    },
                .allowTx = false,
                .lastUsedFreq = 2713500,
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
            },
            (Preset){
                .band =
                    {
                        .bounds = {17400000, 22999999},
                        .name = "174-230",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                        .gainIndex = 18,
                        .squelch = 3,
                        .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
                    },
                .allowTx = false,
                .lastUsedFreq = 20575000,
                .powCalib = {0x8C, 0x8C, 0x8C},
            },
            (Preset){
                .band =
                    {
                        .bounds = {23000000, 31999999},
                        .name = "230-320",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                        .gainIndex = 18,
                        .squelch = 3,
                        .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
                    },
                .allowTx = false,
                .lastUsedFreq = 25355000,
                .powCalib = {0x8C, 0x8C, 0x8C},
            },
            (Preset){
                .band =
                    {
                        .bounds = {32000000, 39999999},
                        .name = "320-400",
                        .step = STEP_25_0kHz,
                        .modulation = MOD_FM,
                        .bw = BK4819_FILTER_BW_WIDE,
                        .gainIndex = 18,
                        .squelch = 3,
                        .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
                    },
                .allowTx = false,
                .lastUsedFreq = 33605000,
                .powCalib = {0x82, 0x82, 0x82},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
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
                .powCalib = {0x8C, 0x8C, 0x8C},
            },
};
// char (*__defpres)[sizeof(defaultPresets)/sizeof(Preset)] = 1;

bool PRESETS_Load();
int8_t PRESETS_Size();
Preset *PRESETS_Item(int8_t i);
int8_t PRESET_IndexOf(Preset *p);
void PRESETS_SelectPresetRelative(bool next);
int8_t PRESET_GetCurrentIndex();
void PRESET_Select(int8_t i);
Preset *PRESET_ByFrequency(uint32_t f);
int8_t PRESET_SelectByFrequency(uint32_t f);
void PRESETS_SavePreset(int8_t num, Preset *p);
void PRESETS_LoadPreset(int8_t num, Preset *p);
void PRESETS_SaveCurrent();
bool PRESET_InRange(const uint32_t f, const Preset *p);
bool PRESET_InRangeOffset(const uint32_t f, const Preset *p);

uint16_t PRESETS_GetStepSize(Preset *p);
uint32_t PRESETS_GetSteps(Preset *p);
uint32_t PRESETS_GetF(Preset *p, uint32_t channel);
uint32_t PRESETS_GetChannel(Preset *p, uint32_t f);

extern Preset defaultPreset;
extern Preset *gCurrentPreset;

#endif /* end of include guard: PRESETLIST_HELPER_H */
