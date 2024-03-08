#include "reset.h"
#include "../driver/st7565.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../settings.h"
#include "../ui/graphics.h"
#include "ARMCM0.h"
#include <string.h>

static uint8_t bandsWrote = 0;
static uint8_t vfosWrote = 0;
static uint16_t channelsWrote = 0;
static bool settingsWrote = 0;
static uint8_t buf[8];
static uint16_t bytesWrote = 0;

static EEPROMType eepromType;

const CH VFOS[] = {
    // VFO 1
    (CH){
        .type = CH_VFO,
        .groups = 1,
        .f = 14550000,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_WIDE,
        .step = STEP_25_0kHz,
        .gainIndex = 16,
        .power = TX_POW_MID,
        .sq.level = 4,
        .sq.closeTime = 1,
        .sq.openTime = 1,
        .vfo.app = APP_MULTIVFO,
    },
    // VFO 2
    (CH){
        .type = CH_VFO,
        .groups = 1,
        .f = 43307500,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_WIDE,
        .step = STEP_25_0kHz,
        .gainIndex = 16,
        .power = TX_POW_MID,
        .sq.level = 4,
        .sq.closeTime = 1,
        .sq.openTime = 1,
    },
    // VFO pro
    (CH){
        .type = CH_VFO,
        .groups = 1,
        .f = 14550000,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_WIDE,
        .step = STEP_25_0kHz,
        .gainIndex = 16,
        .power = TX_POW_MID,
        .sq.level = 4,
        .sq.closeTime = 1,
        .sq.openTime = 1,
    },
    // Analyzer
    (CH){
        .type = CH_VFO,
        .groups = 1,
        .f = 14550000,
        .modulation = MOD_FM,
        .bw = BK4819_FILTER_BW_WIDE,
        .step = STEP_25_0kHz,
        .gainIndex = 16,
        .power = TX_POW_MID,
        .sq.level = 4,
        .sq.closeTime = 1,
        .sq.openTime = 1,
    },
};

void RESET_Init() {
  eepromType = gSettings.eepromType;
  bandsWrote = 0;
  bytesWrote = 0;
  channelsWrote = 0;
  settingsWrote = false;
  memset(buf, 0xFF, sizeof(buf));
}

void RESET_Update() {
  if (!settingsWrote) {
    gSettings = (Settings){
        .checkbyte = EEPROM_CHECKBYTE,
        .eepromType = eepromType,
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
        .contrast = 8,
        .mainApp = APP_MULTIVFO,
        .skipGarbageFrequencies = true,
        .activeCH = 0,
        .activeBand = 22,
        .backlightOnSquelch = BL_SQL_ON,
        .batteryCalibration = 2000,
        .batteryType = BAT_1600,
        .batteryStyle = BAT_PERCENT,
        .nickName = "Anonymous",
        .powCalib =
            {
                {0x8C, 0x8C, 0x8C},
                {0x8C, 0x8C, 0x8C},
                {0x8C, 0x8C, 0x8C}, // stock
                {0x8C, 0x8C, 0x8C},
                {0x82, 0x82, 0x82},
                {0x8C, 0x8C, 0x8C},
                {0x8C, 0x8C, 0x8C},
                {0x8C, 0x8C, 0x8C},
                {0x8C, 0x8C, 0x8C}, // stock END
                {0x8C, 0x8C, 0x8C},
                {0x8C, 0x8C, 0x8C},
                {0x8C, 0x8C, 0x8C},
            },
        .allowTX = TX_ALLOW_LPD_PMR,
    };
    settingsWrote = true;
    bytesWrote += SETTINGS_SIZE;
  } else if (vfosWrote < ARRAY_SIZE(VFOS)) {
    CHANNELS_Save(vfosWrote, &VFOS[vfosWrote]);
    vfosWrote++;
    bytesWrote += sizeof(CH);
  } else if (channelsWrote < CHANNELS_GetCountMax()) {
    CH ch = {
        .name = {0},
        .groups = 0,
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
  uint8_t progressX =
      ConvertDomain(bytesWrote, 0, SETTINGS_GetEEPROMSize(), 1, LCD_WIDTH - 2);
  uint8_t POS_Y = LCD_HEIGHT / 2;

  UI_ClearScreen();
  DrawRect(0, POS_Y, LCD_WIDTH, 10, C_FILL);
  FillRect(1, POS_Y, progressX, 10, C_FILL);
  PrintMediumEx(LCD_XCENTER, POS_Y + 8, POS_C, C_INVERT, "%u%",
                bytesWrote * 100 / SETTINGS_GetEEPROMSize());
}


static App meta = {
    .id = APP_RESET,
    .name = "RESET",
    .init = RESET_Init,
    .update = RESET_Update,
    .render = RESET_Render,
};

App *RESET_Meta() { return &meta; }
