#include "reset.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "../settings.h"
#include "../ui/graphics.h"
#include "ARMCM0.h"
#include "apps.h"
#include <string.h>

static uint8_t presetsWrote = 0;
static bool settingsWrote = 0;
static uint8_t buf[8];
static uint16_t bytesWrote = 0;

static VFO defaultVFOs[2] = {
    (VFO){40655000, 0, "Med lenin", 0, MOD_FM, BK4819_FILTER_BW_WIDE},
    (VFO){40660000, 0, "Med kirov", 0, MOD_FM, BK4819_FILTER_BW_WIDE},
};

static Preset defaultPresets[] = {
    (Preset){
        .band =
            {
                .bounds = {1500000, 3000000},
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
                .bounds = {3000000, 7200000},
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
                .bounds = {7200000, 8800000},
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
                .bounds = {8800000, 10800000},
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
                .bounds = {10800000, 11800000},
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
                .bounds = {11800000, 13500000},
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
                .bounds = {13500000, 14400000},
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
                .bounds = {14400000, 14800000},
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
                .bounds = {14800000, 17400000},
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
                .bounds = {17400000, VHF_UHF_BOUND},
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
                .bounds = {VHF_UHF_BOUND, 35000000},
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
                .bounds = {35000000, 40000000},
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
                .bounds = {40000000, 43307500},
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
                .bounds = {43477500, 44600000},
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
                .bounds = {44600000, 44620000},
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
                .name = "FRS/GMR462",
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
                .bounds = {46272500, 46756250},
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
                .name = "FRS/GMR467",
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
                .bounds = {46770000, 47000000},
                .name = "F12",
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
                .bounds = {47000000, 63000000},
                .name = "F13",
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
                .bounds = {47000000, 63000000},
                .name = "F14",
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
                .bounds = {84000000, 130000000},
                .name = "F15",
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
        .activeChannel = 0,
        .activePreset = 22,
        .presetsCount = ARRAY_SIZE(defaultPresets),
    };
    settingsWrote = true;
    bytesWrote += sizeof(Settings);
  } else if (presetsWrote < ARRAY_SIZE(defaultPresets)) {
    SETTINGS_Save();
    RADIO_SavePreset(presetsWrote, &defaultPresets[presetsWrote]);
    presetsWrote++;
    bytesWrote += sizeof(Preset);
  } else if (bytesWrote < 0x2000) {
    memset(buf, 0xFF, 8);
    EEPROM_WriteBuffer(bytesWrote, buf, 8);
    bytesWrote += 8;
  } else {
    for (uint8_t i = 0; i < 2; i++) {
      RADIO_SaveChannel(i, &defaultVFOs[i]);
    }
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

bool RESET_key(KEY_Code_t k, bool p, bool h) { return false; }
