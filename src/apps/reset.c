#include "reset.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../helper/vfos.h"
#include "../settings.h"
#include "../ui/graphics.h"
#include "ARMCM0.h"
#include <string.h>

static uint32_t bytesMax = 0;
static uint32_t bytesWrote = 0;
static uint16_t channelsMax = 0;
static uint16_t channelsWrote = 0;
static uint8_t presetsWrote = 0;
static uint8_t vfosWrote = 0;
static bool settingsWrote = 0;
static uint8_t buf[8];

static EEPROMType eepromType;

static VFO defaultVFOs[2] = {
    (VFO){
        .rx.f = 14550000,
        .channel = -1,
        .modulation = MOD_PRST,
        .radio = RADIO_UNKNOWN,
    },
    (VFO){
        .rx.f = 43307500,
        .channel = -1,
        .modulation = MOD_PRST,
        .radio = RADIO_UNKNOWN,
    },
};

static EEPROMType determineEepromType() {
  uint8_t bkp[8];
  uint8_t bufr[8];
  EEPROMType type = 0;
  uint8_t buf[8] = {119, 100, 110, 193, 110, 100, 0, 99};
  for (uint8_t i = 0; i < ARRAY_SIZE(EEPROM_SIZES); ++i) {
    uint32_t sz = EEPROM_SIZES[i];
    uint32_t adr = sz - 8;
    EEPROM_ReadBuffer(adr, bkp, 8);
    EEPROM_WriteBuffer(adr, buf, 8);
    EEPROM_ReadBuffer(adr, bufr, 8);
    if (memcmp(buf, bufr, 8) == 0) {
      type = i;
    }
    EEPROM_WriteBuffer(adr, bkp, 8);
  }
  return type;
}

void RESET_Init(void) {
  eepromType = determineEepromType();
  presetsWrote = 0;
  vfosWrote = 0;
  bytesWrote = 0;
  channelsWrote = 0;
  settingsWrote = false;
  memset(buf, 0xFF, sizeof(buf));
  channelsMax = CHANNELS_GetCountMax();
  bytesMax = ARRAY_SIZE(defaultPresets) * PRESET_SIZE + channelsMax * CH_SIZE;
}

void RESET_Update(void) {
  if (!settingsWrote) {
    gSettings = (Settings){
        .eepromType = eepromType,
        .squelch = 4,
        .scrambler = 0,
        .batsave = 4,
        .vox = 0,
        .backlight = BL_TIME_VALUES[3],
        .txTime = 0,
        .micGain = 15,
        .currentScanlist = 15,
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
        .mainApp = APP_VFO2,
        .sqOpenedTimeout = SCAN_TO_NONE,
        .sqClosedTimeout = SCAN_TO_2s,
        .sqlOpenTime = 1,
        .sqlCloseTime = 1,
        .skipGarbageFrequencies = true,
        .scanTimeout = 50,
        .activeVFO = 0,
        .activePreset = 22,
        .presetsCount = ARRAY_SIZE(defaultPresets),
        .backlightOnSquelch = BL_SQL_ON,
        .batteryCalibration = 2000,
        .batteryType = BAT_1600,
        .batteryStyle = BAT_PERCENT,
        .nickName = "Anonymous",
    };
    settingsWrote = true;
    bytesWrote += SETTINGS_SIZE;
  } else if (vfosWrote < ARRAY_SIZE(defaultVFOs)) {
    VFOS_Save(vfosWrote, &defaultVFOs[vfosWrote]);
    vfosWrote++;
    bytesWrote += VFO_SIZE;
  } else if (presetsWrote < ARRAY_SIZE(defaultPresets)) {
    PRESETS_SavePreset(presetsWrote, &defaultPresets[presetsWrote]);
    presetsWrote++;
    bytesWrote += PRESET_SIZE;
  } else if (channelsWrote < channelsMax) {
    CHANNELS_Delete(channelsWrote);
    channelsWrote++;
    bytesWrote += CH_SIZE;
  } else {
    SETTINGS_Save();
    NVIC_SystemReset();
  }
  gRedrawScreen = true;
}

void RESET_Render(void) {
  uint8_t progressX = ConvertDomain(bytesWrote, 0, bytesMax, 1, LCD_WIDTH - 2);
  uint8_t POS_Y = LCD_HEIGHT / 2;

  UI_ClearScreen();
  DrawRect(0, POS_Y, LCD_WIDTH, 10, C_FILL);
  FillRect(1, POS_Y, progressX, 10, C_FILL);
  PrintMedium(0, 16, "%u/%u", channelsWrote, channelsMax);
  PrintMedium(0, 24, "%lu", bytesMax);
  PrintMediumEx(LCD_XCENTER, POS_Y + 8, POS_C, C_INVERT, "%u%",
                bytesWrote * 100 / bytesMax);
}

bool RESET_key(KEY_Code_t k, bool p, bool h) { return true; }
