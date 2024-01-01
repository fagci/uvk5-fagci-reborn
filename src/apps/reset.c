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
static bool settingsWrote = 0;
static uint8_t buf[8];
static uint16_t bytesWrote = 0;

static VFO defaultVFOs[2] = {
    (VFO){
        .fRX = 14550000,
        .channel = 0,
        .isMrMode = false,
        .bw = BK4819_FILTER_BW_WIDE,
        .modulation = MOD_FM,
    },
    (VFO){
        .fRX = 43307500,
        .channel = 0,
        .isMrMode = false,
        .bw = BK4819_FILTER_BW_WIDE,
        .modulation = MOD_FM,
    },
};

static Preset defaultPresets[] = {
    (Preset){
        .band =
            {
                .bounds = {1500000, 2999500},
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
                .bounds = {3000000, 7199500},
                .name = "30-72",
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
                .bounds = {7200000, 8797500},
                .name = "72-88",
                .step = STEP_25_0kHz,
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
                .bounds = {8800000, 10790000},
                .name = "Bcast FM",
                .step = STEP_100_0kHz,
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
                .bounds = {10800000, 11798750},
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
                .bounds = {11800000, 13498750},
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
                .bounds = {13500000, 14398750},
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
                .bounds = {14400000, 14797500},
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
                .bounds = {14800000, 17397500},
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
                .bounds = {17400000, VHF_UHF_BOUND - 2500},
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
                .bounds = {VHF_UHF_BOUND, 34997500},
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
                .bounds = {35000000, 39997500},
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
                .bounds = {40000000, 43305000},
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
                .bounds = {43307500, 43477500},
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
                .bounds = {43480000, 44597500},
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
                .bounds = {44600000, 44618750},
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
                .bounds = {44620000, 46255000},
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
                .bounds = {46256250, 46272500},
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
                .bounds = {46273750, 46756250},
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
                .bounds = {46756250, 46771250},
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
                .bounds = {46775000, 46997500},
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
                .bounds = {47000000, 62997500},
                .name = "470-630",
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
                .bounds = {63000000, 83997500},
                .name = "630-840",
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
                .bounds = {84000000, 86297500},
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
                .bounds = {86300000, 86987200},
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
                .bounds = {87000000, 88997500},
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
                .bounds = {89000000, 95980000},
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
                .bounds = {96000000, 125997500},
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
                .bounds = {126000000, 129997500},
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
        .mainApp = APP_STILL,
        .reserved1 = 0,
        .activeVFO = 0,
        .activePreset = 22,
        .presetsCount = ARRAY_SIZE(defaultPresets),
        .backlightOnSquelch = BL_SQL_ON,
    };
    settingsWrote = true;
    bytesWrote += sizeof(Settings);
  } else if (vfosWrote < ARRAY_SIZE(defaultVFOs)) {
    VFOS_Save(vfosWrote, &defaultVFOs[vfosWrote]);
    vfosWrote++;
    bytesWrote += sizeof(VFO);
  } else if (presetsWrote < ARRAY_SIZE(defaultPresets)) {
    SETTINGS_Save();
    PRESETS_SavePreset(presetsWrote, &defaultPresets[presetsWrote]);
    presetsWrote++;
    bytesWrote += sizeof(Preset);
  } else if (bytesWrote < 0x2000) {
    memset(buf, 0xFF, 8);
    EEPROM_WriteBuffer(bytesWrote, buf, 8);
    bytesWrote += 8;
  } else {
    NVIC_SystemReset();
  }
  gRedrawScreen = true;
}

void RESET_Render() {
  uint8_t progressX = ConvertDomain(bytesWrote, 0, 0x2000, 1, LCD_WIDTH - 2);
  uint8_t POS_Y = LCD_HEIGHT / 2;

  UI_ClearScreen();
  DrawRect(0, POS_Y, LCD_WIDTH, 10, C_FILL);
  FillRect(1, POS_Y, progressX, 10, C_FILL);
  PrintMediumEx(LCD_WIDTH / 2, POS_Y + 8, POS_C, C_INVERT, "%u%",
                bytesWrote * 100 / 0x2000);
}

bool RESET_key(KEY_Code_t k, bool p, bool h) { return true; }
