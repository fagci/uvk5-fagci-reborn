#include "reset.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../helper/vfos.h"
#include "../radio.h"
#include "../settings.h"
#include "../ui/graphics.h"
#include "ARMCM0.h"
#include "apps.h"
#include <string.h>

static uint8_t presetsWrote = 0;
static uint8_t vfosWrote = 0;
static uint16_t channelsWrote = 0;
static bool settingsWrote = 0;
static uint8_t buf[8];
static uint16_t bytesWrote = 0;

static VFO defaultVFOs[2] = {
    (VFO){
        .fRX = 14550000,
        .channel = 0,
        .isMrMode = false,
    },
    (VFO){
        .fRX = 43307500,
        .channel = 0,
        .isMrMode = false,
    },
};

static Preset defaultPresets[] = {
    (Preset){
        .band =
            {
                .bounds = {1500000, 2999999},
                .name = "15-30",
                .step = STEP_5_0kHz,
                .modulation = MOD_AM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {3000000, 6399999},
                .name = "30-64",
                .step = STEP_5_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {6400000, 8799999},
                .name = "64-88",
                .step = STEP_100_0kHz,
                .modulation = MOD_WFM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {8800000, 10799999},
                .name = "Bcast FM",
                .step = STEP_100_0kHz,
                .modulation = MOD_WFM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {10800000, 11799999},
                .name = "108-118",
                .step = STEP_12_5kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {11800000, 13499999},
                .name = "Air",
                .step = STEP_12_5kHz,
                .modulation = MOD_AM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {13500000, 14399999},
                .name = "135-144",
                .step = STEP_12_5kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {14400000, 14799999},
                .name = "2m HAM",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {14800000, 17399999},
                .name = "148-174",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {17400000, VHF_UHF_BOUND2 - 2500},
                .name = "174-BOUND",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {VHF_UHF_BOUND2, 34999999},
                .name = "BOUND-350",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {35000000, 39999999},
                .name = "350-400",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {40000000, 43307499},
                .name = "400-433",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {43307500, 43479999},
                .name = "LPD",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = true,
    },
    (Preset){
        .band =
            {
                .bounds = {43480000, 44599999},
                .name = "434.7-446",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {44600000, 44619999},
                .name = "PMR",
                .step = STEP_6_25kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = true,
    },
    (Preset){
        .band =
            {
                .bounds = {44620000, 46256249},
                .name = "446-462",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {46256250, 46273749},
                .name = "FRS/GM462",
                .step = STEP_12_5kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {46273750, 46756249},
                .name = "462-467",
                .step = STEP_12_5kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {46756250, 46774999},
                .name = "FRS/GM467",
                .step = STEP_12_5kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {46775000, 46999999},
                .name = "468-470",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {47000000, 62000000},
                .name = "470-620",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {84000000, 86299999},
                .name = "840-863",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {86300000, 86999999},
                .name = "LORA",
                .step = STEP_125_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {87000000, 88999999},
                .name = "870-890",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {89000000, 95999999},
                .name = "GSM-900",
                .step = STEP_200_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {96000000, 125999999},
                .name = "960-1260",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {126000000, 129999999},
                .name = "23cm HAM",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {126000000, 134000000},
                .name = "1300-1340",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
                .squelch = 3,
                .squelchType = SQUELCH_RSSI_NOISE_GLITCH,
            },
        .allowTx = false,
    },
};

void RESET_Init() {
  presetsWrote = 0;
  vfosWrote = 0;
  bytesWrote = 0;
  channelsWrote = 0;
  settingsWrote = false;
  memset(buf, 0xFF, sizeof(buf));
}

void RESET_Update() {
  if (!settingsWrote) {
    gSettings = (Settings){
        .squelch = 4,
        .scrambler = 0,
        .batsave = 4,
        .vox = 0,
        .backlight = BL_TIME_VALUES[3],
        .txTime = 0,
        .micGain = 15,
        .currentScanlist = 1,
        .upconverter = UPCONVERTER_OFF,
        .roger = 0,
        .scanmode = 0,
        .chDisplayMode = 0,
        .dw = false,
        .crossBand = false,
        .beep = false,
        .keylock = false,
        .busyChannelTxLock = false,
        .ste = true,
        .repeaterSte = true,
        .dtmfdecode = false,
        .brightness = 8,
        .contrast = 0,
        .mainApp = APP_VFO2,
        .reserved1 = 0,
        .activeVFO = 0,
        .activePreset = 22,
        .presetsCount = ARRAY_SIZE(defaultPresets),
        .backlightOnSquelch = BL_SQL_ON,
        .batteryCalibration = 2000,
        .batteryType = BAT_1600,
        .batteryStyle = BAT_PERCENT,
    };
    settingsWrote = true;
    bytesWrote += sizeof(Settings);
  } else if (vfosWrote < ARRAY_SIZE(defaultVFOs)) {
    VFOS_Save(vfosWrote, &defaultVFOs[vfosWrote]);
    vfosWrote++;
    bytesWrote += sizeof(VFO);
  } else if (presetsWrote < ARRAY_SIZE(defaultPresets)) {
    PRESETS_SavePreset(presetsWrote, &defaultPresets[presetsWrote]);
    presetsWrote++;
    bytesWrote += sizeof(Preset);
  } else if (channelsWrote < CHANNELS_GetCountMax()) {
    CH ch = {
        .name = {0},
        .memoryBanks = 0,
    };
    CHANNELS_Save(channelsWrote, &ch);
    channelsWrote++;
    bytesWrote += sizeof(CH);
  } else {
    SETTINGS_Save();
    NVIC_SystemReset();
  }
  gRedrawScreen = true;
}

void RESET_Render() {
  uint8_t progressX = ConvertDomain(bytesWrote, 0, EEPROM_SIZE, 1, LCD_WIDTH - 2);
  uint8_t POS_Y = LCD_HEIGHT / 2;

  UI_ClearScreen();
  DrawRect(0, POS_Y, LCD_WIDTH, 10, C_FILL);
  FillRect(1, POS_Y, progressX, 10, C_FILL);
  PrintMediumEx(LCD_XCENTER, POS_Y + 8, POS_C, C_INVERT, "%u%",
                bytesWrote * 100 / EEPROM_SIZE);
}

bool RESET_key(KEY_Code_t k, bool p, bool h) { return true; }
