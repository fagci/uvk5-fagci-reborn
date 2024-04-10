#include "si473x.h"
#include "../inc/dp32g030/gpio.h"
#include "audio.h"
#include "gpio.h"
#include "i2c.h"
#include "system.h"
#include "systick.h"

#define SI4732_DELAY_MS 400

#define RST_HIGH GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BK1080)
#define RST_LOW GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BK1080)

// Si4735 property codes
// Check "Si47xx Programming Guide" to determine which chips support which
// properties.
enum {
  PROP_GPO_IEN = 0x0001,
  PROP_DIGITAL_OUTPUT_FORMAT = 0x0102,
  PROP_DIGITAL_OUTPUT_SAMPLE_RATE = 0x0104,
  PROP_REFCLK_FREQ = 0x0201,
  PROP_REFCLK_PRESCALE = 0x0202,
  PROP_RX_VOLUME = 0x4000,
  PROP_RX_HARD_MUTE = 0x4001,
  // FM mode
  PROP_FM_DEEMPHASIS = 0x1100,
  PROP_FM_CHANNEL_FILTER = 0x1102,
  PROP_FM_BLEND_STEREO_THRESHOLD = 0x1105,
  PROP_FM_BLEND_MONO_THRESHOLD = 0x1106,
  PROP_FM_MAX_TUNE_ERROR = 0x1108,
  PROP_FM_RSQ_INT_SOURCE = 0x1200,
  PROP_FM_RSQ_SNR_HI_THRESHOLD = 0x1201,
  PROP_FM_RSQ_SNR_LO_THRESHOLD = 0x1202,
  PROP_FM_RSQ_RSSI_HI_THRESHOLD = 0x1203,
  PROP_FM_RSQ_RSSI_LO_THRESHOLD = 0x1204,
  PROP_FM_RSQ_MULTIPATH_HI_THRESHOLD = 0x1205,
  PROP_FM_RSQ_MULTIPATH_LO_THRESHOLD = 0x1206,
  PROP_FM_RSQ_BLEND_THRESHOLD = 0x1207,
  PROP_FM_SOFT_MUTE_RATE = 0x1300,
  PROP_FM_SOFT_MUTE_SLOPE = 0x1301,
  PROP_FM_SOFT_MUTE_MAX_ATTENUATION = 0x1302,
  PROP_FM_SOFT_MUTE_SNR_THRESHOLD = 0x1303,
  PROP_FM_SOFT_MUTE_RELEASE_RATE = 0x1304,
  PROP_FM_SOFT_MUTE_ATTACK_RATE = 0x1305,
  PROP_FM_SEEK_BAND_BOTTOM = 0x1400,
  PROP_FM_SEEK_BAND_TOP = 0x1401,
  PROP_FM_SEEK_FREQ_SPACING = 0x1402,
  PROP_FM_SEEK_TUNE_SNR_THRESHOLD = 0x1403,
  PROP_FM_SEEK_TUNE_RSSI_THRESHOLD = 0x1404,
  PROP_FM_RDS_INT_SOURCE = 0x1500,
  PROP_FM_RDS_INT_FIFO_COUNT = 0x1501,
  PROP_FM_RDS_CONFIG = 0x1502,
  PROP_FM_RDS_CONFIDENCE = 0x1503,
  PROP_FM_BLEND_RSSI_STEREO_THRESHOLD = 0x1800,
  PROP_FM_BLEND_RSSI_MONO_THRESHOLD = 0x1801,
  PROP_FM_BLEND_RSSI_ATTACK_RATE = 0x1802,
  PROP_FM_BLEND_RSSI_RELEASE_RATE = 0x1803,
  PROP_FM_BLEND_SNR_STEREO_THRESHOLD = 0x1804,
  PROP_FM_BLEND_SNR_MONO_THRESHOLD = 0x1805,
  PROP_FM_BLEND_SNR_ATTACK_RATE = 0x1806,
  PROP_FM_BLEND_SNR_RELEASE_RATE = 0x1807,
  PROP_FM_BLEND_MULTIPATH_STEREO_THRESHOLD = 0x1808,
  PROP_FM_BLEND_MULTIPATH_MONO_THRESHOLD = 0x1809,
  PROP_FM_BLEND_MULTIPATH_ATTACK_RATE = 0x180A,
  PROP_FM_BLEND_MULTIPATH_RELEASE_RATE = 0x180B,
  PROP_FM_HICUT_SNR_HIGH_THRESHOLD = 0x1A00,
  PROP_FM_HICUT_SNR_LOW_THRESHOLD = 0x1A01,
  PROP_FM_HICUT_ATTACK_RATE = 0x1A02,
  PROP_FM_HICUT_RELEASE_RATE = 0x1A03,
  PROP_FM_HICUT_MULTIPATH_TRIGGER_THRESHOLD = 0x1A04,
  PROP_FM_HICUT_MULTIPATH_END_THRESHOLD = 0x1A05,
  PROP_FM_HICUT_CUTOFF_FREQUENCY = 0x1A06,
  // AM mode
  PROP_AM_DEEMPHASIS = 0x3100,
  PROP_AM_CHANNEL_FILTER = 0x3102,
  PROP_AM_AUTOMATIC_VOLUME_CONTROL_MAX_GAIN = 0x3103,
  PROP_AM_MODE_AFC_SW_PULL_IN_RANGE = 0x3104,
  PROP_AM_MODE_AFC_SW_LOCK_IN_RANGE = 0x3105,
  PROP_AM_RSQ_INT_SOURCE = 0x3200,
  PROP_AM_RSQ_SNR_HIGH_THRESHOLD = 0x3201,
  PROP_AM_RSQ_SNR_LOW_THRESHOLD = 0x3202,
  PROP_AM_RSQ_RSSI_HIGH_THRESHOLD = 0x3203,
  PROP_AM_RSQ_RSSI_LOW_THRESHOLD = 0x3204,
  PROP_AM_SOFT_MUTE_RATE = 0x3300,
  PROP_AM_SOFT_MUTE_SLOPE = 0x3301,
  PROP_AM_SOFT_MUTE_MAX_ATTENUATION = 0x3302,
  PROP_AM_SOFT_MUTE_SNR_THRESHOLD = 0x3303,
  PROP_AM_SOFT_MUTE_RELEASE_RATE = 0x3304,
  PROP_AM_SOFT_MUTE_ATTACK_RATE = 0x3305,
  PROP_AM_SEEK_BAND_BOTTOM = 0x3400,
  PROP_AM_SEEK_BAND_TOP = 0x3401,
  PROP_AM_SEEK_FREQ_SPACING = 0x3402,
  PROP_AM_SEEK_TUNE_SNR_THRESHOLD = 0x3403,
  PROP_AM_SEEK_TUNE_RSSI_THRESHOLD = 0x3404,
  PROP_AM_FRONTEND_AGC_CONTROL = 0x3705,
  // WB mode - not Si4735
  PROP_WB_MAX_TUNE_ERROR = 0x5108,
  PROP_WB_RSQ_INT_SOURCE = 0x5200,
  PROP_WB_RSQ_SNR_HI_THRESHOLD = 0x5201,
  PROP_WB_RSQ_SNR_LO_THRESHOLD = 0x5202,
  PROP_WB_RSQ_RSSI_HI_THRESHOLD = 0x5203,
  PROP_WB_RSQ_RSSI_LO_THRESHOLD = 0x5204,
  PROP_WB_VALID_SNR_THRESHOLD = 0x5403,
  PROP_WB_VALID_RSSI_THRESHOLD = 0x5404,
  PROP_WB_SAME_INT_SOURCE = 0x5500, // Si4707 only
  PROP_WB_ASQ_INT_SOURCE = 0x5600,
  // AUX mode - Si4735-D60 or later
  PROP_AUX_ASQ_INT_SOURCE = 0x6600,
};

// Si4735 command codes
enum {
  CMD_POWER_UP = 0x01,
  CMD_GET_REV = 0x10,
  CMD_POWER_DOWN = 0x11,
  CMD_SET_PROPERTY = 0x12,
  CMD_GET_PROPERTY = 0x13,
  CMD_GET_INT_STATUS = 0x14,
  CMD_PATCH_ARGS = 0x15,
  CMD_PATCH_DATA = 0x16,
  CMD_GPIO_CTL = 0x80,
  CMD_GPIO_SET = 0x81,
  // FM mode
  CMD_FM_TUNE_FREQ = 0x20,
  CMD_FM_SEEK_START = 0x21,
  CMD_FM_TUNE_STATUS = 0x22,
  CMD_FM_RSQ_STATUS = 0x23,
  CMD_FM_RDS_STATUS = 0x24,
  CMD_FM_AGC_STATUS = 0x27,
  CMD_FM_AGC_OVERRIDE = 0x28,
  // AM mode
  CMD_AM_TUNE_FREQ = 0x40,
  CMD_AM_SEEK_START = 0x41,
  CMD_AM_TUNE_STATUS = 0x42,
  CMD_AM_RSQ_STATUS = 0x43,
  CMD_AM_AGC_STATUS = 0x47,
  CMD_AM_AGC_OVERRIDE = 0x48,
  // WB mode - not Si4735
  CMD_WB_TUNE_FREQ = 0x50,
  CMD_WB_TUNE_STATUS = 0x52,
  CMD_WB_RSQ_STATUS = 0x53,
  CMD_WB_SAME_STATUS = 0x54, // Si4707 only
  CMD_WB_ASQ_STATUS = 0x55,
  CMD_WB_AGC_STATUS = 0x57,
  CMD_WB_AGC_OVERRIDE = 0x58,
  // AUX mode - Si4735-D60 or later
  CMD_AUX_ASRC_START = 0x61,
  CMD_AUX_ASQ_STATUS = 0x65,
};

// Command arguments
enum {
  // POWER_UP
  /* See POWER_UP_AUDIO_OUT constants above for ARG2. */
  POWER_UP_ARG1_CTSIEN = 0b10000000,  // CTS interrupt enable
  POWER_UP_ARG1_GPO2OEN = 0b01000000, // GPO2/INT output enable
  POWER_UP_ARG1_PATCH = 0b00100000,   // Patch enable
  POWER_UP_ARG1_XOSCEN =
      0b00010000, // Enable internal oscillator with external 32768 Hz crystal
  POWER_UP_ARG1_FUNC_FM = 0x0,  // FM receive mode
  POWER_UP_ARG1_FUNC_AM = 0x1,  // AM receive mode
  POWER_UP_ARG1_FUNC_TX = 0x2,  // FM transmit mode - not Si4735 or Si4707
  POWER_UP_ARG1_FUNC_WB = 0x3,  // WB receive mode - not Si4735
  POWER_UP_ARG1_FUNC_AUX = 0x4, // Auxiliary input mode - Si4735-D60 or later
  POWER_UP_ARG1_FUNC_REV = 0xF, // Query chip's hardware and firmware revisions
  // FM_TUNE_FREQ, AM_TUNE_FREQ
  FM_TUNE_FREQ_ARG1_FREEZE = 0b10,
  TUNE_FREQ_ARG1_FAST = 0b01, // Fast, inaccurate tune
  // FM_SEEK_START, AM_SEEK_START
  SEEK_START_ARG1_SEEK_UP = 0b1000, // 1 = Seek up, 0 = Seek down
  SEEK_START_ARG1_WRAP = 0b0100,    // Wrap when band limit reached
  // FM_TUNE_STATUS, AM_TUNE_STATUS, WB_TUNE_STATUS
  TUNE_STATUS_ARG1_CANCEL_SEEK = 0b10, // Cancel seek operation - not WB
  TUNE_STATUS_ARG1_CLEAR_INT = 0b01,   // Clear STC interrupt
  // FM_RSQ_STATUS, AM_RSQ_STATUS, WB_RSQ_STATUS
  RSQ_STATUS_ARG1_CLEAR_INT = 0b1, // Clear RSQ and related interrupts
  // FM_RDS_STATUS
  RDS_STATUS_ARG1_STATUS_ONLY = 0b100,
  RDS_STATUS_ARG1_CLEAR_FIFO = 0b010, // Clear RDS receive FIFO
  RDS_STATUS_ARG1_CLEAR_INT = 0b001,  // Clear RDS interrupt
  // WB_SAME_STATUS
  SAME_STATUS_ARG1_CLEAR_BUFFER = 0b10, // Clear SAME receive buffer
  SAME_STATUS_ARG1_CLEAR_INT = 0b01,    // Clear SAME interrupt
  // AUX_ASQ_STATUS, WB_ASQ_STATUS
  ASQ_STATUS_ARG1_CLEAR_INT = 0b1, // Clear ASQ interrupt
  // FM_AGC_OVERRIDE, AM_AGC_OVERRIDE, WB_AGC_OVERRIDE
  AGC_OVERRIDE_ARG1_DISABLE_AGC = 0b1, // Disable AGC
  // GPIO_CTL, GPIO_SET
  GPIO_ARG1_GPO3 = 0b1000, // GPO3
  GPIO_ARG1_GPO2 = 0b0100, // GPO2
  GPIO_ARG1_GPO1 = 0b0010, // GPO1
};

RSQStatus rsqStatus;

static SI4732_MODE si4732mode = SI4732_FM;
static uint32_t currentFreq = 10320;

static const uint8_t SI4732_I2C_ADDR = 0x22;

void SI4732_ReadBuffer(uint8_t *buf, uint8_t size) {
  I2C_Start();
  I2C_Write(SI4732_I2C_ADDR + 1);
  I2C_ReadBuffer(buf, size);
  I2C_Stop();
}

void SI4732_WriteBuffer(uint8_t *buf, uint8_t size) {
  I2C_Start();
  I2C_Write(SI4732_I2C_ADDR);
  I2C_WriteBuffer(buf, size);
  I2C_Stop();
}

void waitToSend() {
  uint8_t tmp = 0;
  SI4732_ReadBuffer((uint8_t *)&tmp, 1);
  while (!(tmp & SI4735_STATUS_CTS)) {
    SYSTICK_DelayUs(100);
    SI4732_ReadBuffer((uint8_t *)&tmp, 1);
  }
}

void sendProperty(uint16_t prop, uint16_t parameter) {
  waitToSend();
  uint8_t tmp[6] = {CMD_SET_PROPERTY, 0, prop >> 8, prop & 0xff, parameter >> 8,
                    parameter & 0xff};
  SI4732_WriteBuffer(tmp, 6);
  // SYSTEM_DelayMs(550);
  // SYSTEM_DelayMs(100);
}

void RSQ_GET() {
  waitToSend();
  if (si4732mode == SI4732_AM) {
    uint8_t cmd[2] = {CMD_AM_RSQ_STATUS, 0x01};
    SI4732_WriteBuffer(cmd, 2);
  } else if (si4732mode == SI4732_FM) {
    uint8_t cmd[2] = {CMD_FM_RSQ_STATUS, 0x01};
    SI4732_WriteBuffer(cmd, 2);
  }

  SI4732_ReadBuffer(rsqStatus.raw, si4732mode == SI4732_FM ? 8 : 6);
}

void enableRDS(void) {
  // Enable and configure RDS reception
  if (si4732mode == SI4732_FM) {
    sendProperty(SI4735_PROP_FM_RDS_INT_SOURCE, SI4735_FLG_RDSRECV);
    // Set the FIFO high-watermark to 12 RDS blocks, which is safe even for
    // old chips, yet large enough to improve performance.
    sendProperty(SI4735_PROP_FM_RDS_INT_FIFO_COUNT, 12);
    sendProperty(SI4735_PROP_FM_RDS_CONFIG,
                 ((SI4735_FLG_BLETHA_35 | SI4735_FLG_BLETHB_35 |
                   SI4735_FLG_BLETHC_35 | SI4735_FLG_BLETHD_35)
                  << 8) |
                     SI4735_FLG_RDSEN);
  };
}

void SI4732_PowerUp() {
  waitToSend();
  uint8_t cmd[3] = {SI4735_CMD_POWER_UP, 0x10 | SI4735_FUNC_FM,
                    SI4735_OUT_ANALOG};
  if (si4732mode == SI4732_AM) {
    cmd[1] = 0x10 | SI4735_FUNC_AM;
  }
  SI4732_WriteBuffer(cmd, 3);
  SYSTEM_DelayMs(10);

  enableRDS();
}

void SI4732_PowerDown() {
  AUDIO_ToggleSpeaker(false);
  uint8_t cmd[1] = {0x11};
  waitToSend();
  SI4732_WriteBuffer(cmd, 1);
  waitToSend();
  SYSTICK_Delay250ns(1);
  RST_LOW;
}

void SI4732_SwitchMode() { SI4732_PowerDown(); }

void SI4732_SetFreq(uint32_t freq) {
  currentFreq = freq;
  uint8_t hb = (freq >> 8) & 0xFF;
  uint8_t lb = freq & 0xFF;

  waitToSend();
  if (si4732mode == SI4732_FM) {
    uint8_t cmd[5] = {CMD_FM_TUNE_FREQ, 0x00, hb, lb};
    SI4732_WriteBuffer(cmd, 4);

  } else if (si4732mode == SI4732_AM) {
    uint8_t cmd[6] = {CMD_AM_TUNE_FREQ, 0x00, hb, lb, 0};
    SI4732_WriteBuffer(cmd, 5);
  }

  SYSTEM_DelayMs(80);
  RSQ_GET();
}

void setVolume(uint8_t volume) {
  if (volume < 0)
    volume = 0;
  if (volume > 63)
    volume = 63;
  sendProperty(PROP_RX_VOLUME, volume);
}

void setAvcAmMaxGain(uint8_t gain) {
  if (gain < 12 || gain > 90)
    return;
  sendProperty(PROP_AM_AUTOMATIC_VOLUME_CONTROL_MAX_GAIN, gain * 340);
}

void AM_FRONTEND_AGC_CONTROL(uint8_t MIN_GAIN_INDEX, uint8_t ATTN_BACKUP) {
  uint16_t num = MIN_GAIN_INDEX << 8 | ATTN_BACKUP;
  sendProperty(PROP_AM_FRONTEND_AGC_CONTROL, num);
}

void setAmSoftMuteMaxAttenuation(uint8_t smattn) {
  sendProperty(PROP_AM_SOFT_MUTE_MAX_ATTENUATION, smattn);
}

void setBandwidth(uint8_t AMCHFLT, uint8_t AMPLFLT) {
  uint8_t tmp[6] = {0x12, 0, 0x31, 0x02, AMCHFLT, AMPLFLT};
  waitToSend();
  SI4732_WriteBuffer(tmp, 6);
}

void SI4732_Init() {
  RST_LOW;
  SYSTEM_DelayMs(10);
  RST_HIGH;
  SYSTEM_DelayMs(10);

  SI4732_PowerUp();

  AUDIO_ToggleSpeaker(true);

  setVolume(63);

  SI4732_SetFreq(currentFreq);
}

void SI4732_ReadRDS(uint8_t buf[13]) {
  uint8_t cmd[2] = {CMD_FM_RDS_STATUS, RDS_STATUS_ARG1_CLEAR_INT};
  waitToSend();
  SI4732_WriteBuffer(cmd, 2);
  SI4732_ReadBuffer(buf, 13);
}
