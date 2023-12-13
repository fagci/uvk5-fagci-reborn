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

static Preset defaultPresets[] = {
    (Preset){
        .band =
            {
                .bounds = {1500000, 3000000},
                .name = "F1",
                .step = STEP_5_0kHz,
                .modulation = MOD_AM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 90,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {3000000, 7200000},
                .name = "F2",
                .step = STEP_5_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 90,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {7200000, 8800000},
                .name = "F3",
                .step = STEP_5_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 90,
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
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {10800000, 11800000},
                .name = "F5",
                .step = STEP_12_5kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
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
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {13500000, 14400000},
                .name = "F6",
                .step = STEP_12_5kHz,
                .modulation = MOD_AM,
                .bw = BK4819_FILTER_BW_NARROW,
                .gainIndex = 90,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {14400000, 14800000},
                .name = "F6",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {14800000, 17400000},
                .name = "F6",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {17400000, VHF_UHF_BOUND},
                .name = "F6",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {VHF_UHF_BOUND, 35000000},
                .name = "F6",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {35000000, 40000000},
                .name = "F6",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {40000000, 47000000},
                .name = "F6",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {47000000, 63000000},
                .name = "F6",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
            },
        .allowTx = false,
    },
    (Preset){
        .band =
            {
                .bounds = {84000000, 130000000},
                .name = "F6",
                .step = STEP_25_0kHz,
                .modulation = MOD_FM,
                .bw = BK4819_FILTER_BW_WIDE,
                .gainIndex = 90,
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
    NVIC_SystemReset();
  }
  gRedrawScreen = true;
}

void RESET_Render() {
  UI_ClearScreen();
  PrintMedium(0, 2 * 8 + 12, "%u%", bytesWrote * 100 / 0x2000);

  memset(gFrameBuffer[3], 0b00111100,
         ConvertDomain(bytesWrote, 0, 0x2000, 0, LCD_WIDTH));
}

bool RESET_key(KEY_Code_t k, bool p, bool h) { return false; }
