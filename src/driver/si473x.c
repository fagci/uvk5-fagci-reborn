#include "si473x.h"
#include "../inc/dp32g030/gpio.h"
#include "audio.h"
#include "gpio.h"
#include "i2c.h"
#include "system.h"
#include "systick.h"
#include <string.h>

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

// Command responses
// Names that begin with FIELD are argument masks.  Others are argument
// constants.
enum {
  // FM_TUNE_STATUS, AM_TUNE_STATUS, WB_TUNE_STATUS
  FIELD_TUNE_STATUS_RESP1_SEEK_LIMIT =
      0b10000000,                            // Seek hit search limit - not WB
  FIELD_TUNE_STATUS_RESP1_AFC_RAILED = 0b10, // AFC railed
  FIELD_TUNE_STATUS_RESP1_SEEKABLE =
      0b01, // Station could currently be found by seek,
  FIELD_TUNE_STATUS_RESP1_VALID = 0b01, // that is, the station is valid
  // FM_RSQ_STATUS, AM_RSQ_STATUS, WB_RSQ_STATUS
  /* See RSQ interrupts above for RESP1. */
  FIELD_RSQ_STATUS_RESP2_SOFT_MUTE = 0b1000,  // Soft mute active - not WB
  FIELD_RSQ_STATUS_RESP2_AFC_RAILED = 0b0010, // AFC railed
  FIELD_RSQ_STATUS_RESP2_SEEKABLE =
      0b0001, // Station could currently be found by seek,
  FIELD_RSQ_STATUS_RESP2_VALID = 0b0001,      // that is, the station is valid
  FIELD_RSQ_STATUS_RESP3_STEREO = 0b10000000, // Stereo pilot found - FM only
  FIELD_RSQ_STATUS_RESP3_STEREO_BLEND =
      0b01111111, // Stereo blend in % (100 = full stereo, 0 = full mono) - FM
                  // only
  // FM_RDS_STATUS
  /* See RDS interrupts above for RESP1. */
  FIELD_RDS_STATUS_RESP2_FIFO_OVERFLOW = 0b00000100, // FIFO overflowed
  FIELD_RDS_STATUS_RESP2_SYNC = 0b00000001, // RDS currently synchronized
  FIELD_RDS_STATUS_RESP12_BLOCK_A = 0b11000000,
  FIELD_RDS_STATUS_RESP12_BLOCK_B = 0b00110000,
  FIELD_RDS_STATUS_RESP12_BLOCK_C = 0b00001100,
  FIELD_RDS_STATUS_RESP12_BLOCK_D = 0b00000011,
  RDS_STATUS_RESP12_BLOCK_A_NO_ERRORS = 0U << 6,     // Block had no errors
  RDS_STATUS_RESP12_BLOCK_A_2_BIT_ERRORS = 1U << 6,  // Block had 1-2 bit errors
  RDS_STATUS_RESP12_BLOCK_A_5_BIT_ERRORS = 2U << 6,  // Block had 3-5 bit errors
  RDS_STATUS_RESP12_BLOCK_A_UNCORRECTABLE = 3U << 6, // Block was uncorrectable
  RDS_STATUS_RESP12_BLOCK_B_NO_ERRORS = 0U << 4,
  RDS_STATUS_RESP12_BLOCK_B_2_BIT_ERRORS = 1U << 4,
  RDS_STATUS_RESP12_BLOCK_B_5_BIT_ERRORS = 2U << 4,
  RDS_STATUS_RESP12_BLOCK_B_UNCORRECTABLE = 3U << 4,
  RDS_STATUS_RESP12_BLOCK_C_NO_ERRORS = 0U << 2,
  RDS_STATUS_RESP12_BLOCK_C_2_BIT_ERRORS = 1U << 2,
  RDS_STATUS_RESP12_BLOCK_C_5_BIT_ERRORS = 2U << 2,
  RDS_STATUS_RESP12_BLOCK_C_UNCORRECTABLE = 3U << 2,
  RDS_STATUS_RESP12_BLOCK_D_NO_ERRORS = 0U << 0,
  RDS_STATUS_RESP12_BLOCK_D_2_BIT_ERRORS = 1U << 0,
  RDS_STATUS_RESP12_BLOCK_D_5_BIT_ERRORS = 2U << 0,
  RDS_STATUS_RESP12_BLOCK_D_UNCORRECTABLE = 3U << 0,
  // WB_SAME_STATUS - TODO

  // AUX_ASQ_STATUS, WB_ASQ_STATUS
  /* See ASQ interrupts above for RESP1. */
  FIELD_AUX_ASQ_STATUS_RESP2_OVERLOAD =
      0b1, // Audio input is currently overloading ADC
  FIELD_WB_ASQ_STATUS_RESP2_ALERT = 0b1, // Alert tone is present
  // FM_AGC_STATUS, AM_AGC_STATUS, WB_AGC_STATUS
  FIELD_AGC_STATUS_RESP1_DISABLE_AGC = 0b1, // True if AGC disabled
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

static SI4732_MODE si4732mode = SI4732_FM;
static uint8_t rssi = 0;
static uint8_t snr = 0;
static uint32_t currentFreq = 10320;

uint8_t SI4732_GetRSSI() { return rssi; }
uint8_t SI4732_GetSNR() { return snr; }

void SI4732_ReadBuffer(uint8_t *Value, uint8_t size) {
  I2C_Start();
  I2C_Write(0x23);
  I2C_ReadBuffer(Value, size);
  I2C_Stop();
}

void SI4732_WriteBuffer(uint8_t *buff, uint8_t size) {
  I2C_Start();
  I2C_Write(0x22);
  I2C_WriteBuffer(buff, size);
  I2C_Stop();
}

void waitToSend() {
  uint8_t tmp = 0;
  do {
    SYSTICK_DelayUs(300);
    SI4732_ReadBuffer((uint8_t *)&tmp, 1);
  } while (!(tmp & 0x80) || tmp & 0x40);
}

void sendProperty(uint16_t propertyNumber, uint16_t parameter) {
  waitToSend();

  uint8_t tmp[6] = {0x12,
                    0,
                    propertyNumber >> 8,
                    propertyNumber & 0xff,
                    parameter >> 8,
                    parameter & 0xff};
  SI4732_WriteBuffer(tmp, 6);
  SYSTEM_DelayMs(550);
}

void SI4732_WAIT_STATUS(uint8_t state) {
  uint8_t tmp = 0;
  do {
    SI4732_ReadBuffer((uint8_t *)&tmp, 1);
  } while (tmp != state);
}

void RSQ_GET() {
  // AM_RSQ_STATUS
  //    GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
  waitToSend();
  if (si4732mode == SI4732_AM) {
    uint8_t cmd[2] = {CMD_AM_RSQ_STATUS, 0x00};
    SI4732_WriteBuffer(cmd, 2);
  } else if (si4732mode == SI4732_FM) {
    uint8_t cmd[2] = {CMD_FM_RSQ_STATUS, 0x00};
    SI4732_WriteBuffer(cmd, 2);
  }
  waitToSend();

  uint8_t cmd_read[6] = {0};
  SI4732_ReadBuffer(cmd_read, 6);
  rssi = cmd_read[4];
  snr = cmd_read[5];
  //    GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
}

enum {
  CTS_MASK = 0b10000000,  // Clear To Send
  ERR_MASK = 0b01000000,  // Error occurred
  RSQ_MASK = 0b00001000,  // Received Signal Quality measurement has triggered
  RDS_MASK = 0b00000100,  // RDS data received (FM mode only)
  SAME_MASK = 0b00000100, // SAME (WB) data received (Si4707 only)
  ASQ_MASK = 0b00000010,  // Audio Signal Quality (AUX and WB modes only)
  STC_MASK = 0b00000001   // Seek/Tune Complete
};

void enableRDS(void) {
  // Enable and configure RDS reception
  if (si4732mode == SI4732_FM) {
    sendProperty(SI4735_PROP_FM_RDS_INT_SOURCE, SI4735_FLG_RDSRECV);
    // Set the FIFO high-watermark to 12 RDS blocks, which is safe even for
    // old chips, yet large enough to improve performance.
    sendProperty(SI4735_PROP_FM_RDS_INT_FIFO_COUNT, 0x0C);
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

  /* uint16_t int_mask; // Interrupts to enable
  if (si4732mode == SI4732_FM) {
    int_mask = CTS_MASK | RDS_MASK;
  } else { // AM, SW, LW, and FM without RDS
    int_mask = STC_MASK | RSQ_MASK;
  }
  sendProperty(PROP_GPO_IEN, int_mask); */
}

void SI4732_PowerDown() {
  AUDIO_ToggleSpeaker(false);
  uint8_t cmd[1] = {0x11};
  SI4732_WriteBuffer(cmd, 1);
  SI4732_WAIT_STATUS(0x80);
  SYSTICK_Delay250ns(1);
  RST_LOW;
}
si47x_rds_status rdsResponse = {0};

typedef union {
  struct {
    uint8_t INTACK : 1; // Interrupt Acknowledge; 0 = RDSINT status preserved; 1
                        // = Clears RDSINT.
    uint8_t MTFIFO : 1; // Empty FIFO; 0 = If FIFO not empty; 1 = Clear RDS
                        // Receive FIFO.
    uint8_t STATUSONLY : 1; // Determines if data should be removed from the RDS
                            // FIFO.
    uint8_t dummy : 5;
  } arg;
  uint8_t raw;
} si47x_rds_command;

enum {
  PI_H = 4, // Also "Block A"
  PI_L,
  Block_B_H,
  Block_B_L,
  Block_C_H,
  Block_C_L,
  Block_D_H,
  Block_D_L
};

RDS rds;

#define MAKE_WORD(hb, lb) (((uint8_t)(hb) << 8U) | (uint8_t)lb)

enum { NO_DATE_TIME = 127 };

enum {
  RDS_THRESHOLD = 3,     // Threshold for larger variables
  RDS_BOOL_THRESHOLD = 7 // Threshold for boolean variables
};

static char make_printable(char ch) {
  // Replace non-ASCII char with space
  if (ch < 32 || 126 < ch)
    ch = ' ';
  return ch;
}

/* RDS and RBDS data */
static ternary _abRadioText;       // Indicates new radioText[] string
static ternary _abProgramTypeName; // Indicates new programTypeName[] string
/* RDS data counters */
static uint8_t _extendedCountryCode_count;
static uint8_t _language_count;

bool SI4732_GetRDS() {

  bool new_info = false;
  uint8_t segment;

  while (1) {
    waitToSend();
    // Ask for next RDS group and clear RDS interrupt
    uint8_t cmd[2] = {CMD_FM_RDS_STATUS, RDS_STATUS_ARG1_CLEAR_INT};
    SI4732_WriteBuffer(cmd, 2);
    waitToSend();

    SI4732_ReadBuffer(rdsResponse.raw, 13);

    // Check for RDS signal
    rds.RDSSignal = rdsResponse.raw[2] & FIELD_RDS_STATUS_RESP2_SYNC;
    // Get number of RDS groups (packets) available
    uint8_t num_groups = rdsResponse.raw[3];
    // Stop if nothing returned
    if (!num_groups)
      break;

    /* Because PI is resent in every packet's Block A, we told the radio its OK
     * to give us packets with a corrupted Block A.
     */
    // Check if PI received is valid
    if ((rdsResponse.raw[12] & FIELD_RDS_STATUS_RESP12_BLOCK_A) !=
        RDS_STATUS_RESP12_BLOCK_A_UNCORRECTABLE) {
      // Get PI code
      rds.programId = MAKE_WORD(rdsResponse.raw[PI_H], rdsResponse.raw[PI_L]);
    }
    // Get PTY code
    rds.programType = ((rdsResponse.raw[Block_B_H] & 0b00000011) << 3U) |
                      (rdsResponse.raw[Block_B_L] >> 5U);
    // Get Traffic Program bit
    rds.trafficProgram = (bool)(rdsResponse.raw[Block_B_H] & 0b00000100);

    // Get group type (0-15)
    uint8_t type = rdsResponse.raw[Block_B_H] >> 4U;
    // Get group version (0=A, 1=B)
    bool version = rdsResponse.raw[Block_B_H] & 0b00001000;

    // Save which group type and version was received
    if (version) {
      rds.groupB |= 1U << type;
    } else {
      rds.groupA |= 1U << type;
    }

    // Groups 0A & 0B - Basic tuning and switching information
    // Group 15B - Fast basic tuning and switching information
    /* Note: We support both Groups 0 and 15B in case the station has poor
     * reception and RDS packets are barely getting through.  This increases
     * the chances of receiving this info.
     */
    if (type == 0 || (type == 15 && version == 1)) {
      // Various flags
      rds.trafficAlert = (bool)(rdsResponse.raw[Block_B_L] & 0b00010000);
      rds.music = (bool)(rdsResponse.raw[Block_B_L] & 0b00001000);
      bool DI = rdsResponse.raw[Block_B_L] & 0b00000100;

      // Get segment number
      segment = rdsResponse.raw[Block_B_L] & 0b00000011;
      // Handle DI code
      switch (segment) {
      case 0:
        rds.dynamicPTY = DI;
        break;
      case 1:
        rds.compressedAudio = DI;
        break;
      case 2:
        rds.binauralAudio = DI;
        break;
      case 3:
        rds.RDSStereo = DI;
        break;
      }

      // Groups 0A & 0B
      if (type == 0) {
        // Program Service
        char *ps = &rds.programService[segment * 2];
        *ps++ = make_printable(rdsResponse.raw[Block_D_H]);
        *ps = make_printable(rdsResponse.raw[Block_D_L]);
      }
      new_info = true;
    }
    // Group 1A - Extended Country Code (ECC) and Language Code
    else if (type == 1 && version == 0) {
      // We are only interested in the Extended Country Code (ECC) and
      // Language Code for this Group.

      // Get Variant code
      switch (rdsResponse.raw[Block_C_H] & 0b01110000) {
      case (0 << 4): // Variant==0
        // Extended Country Code
        // Check if count has reached threshold
        if (_extendedCountryCode_count < RDS_THRESHOLD) {
          uint8_t ecc = rdsResponse.raw[Block_C_L];
          // Check if datum changed
          if (rds.extendedCountryCode != ecc) {
            _extendedCountryCode_count = 0;
            new_info = true;
          }
          // Save new data
          rds.extendedCountryCode = ecc;
          ++_extendedCountryCode_count;
        }
        break;
      case (3 << 4): // Variant==3
        // Language Code
        // Check if count has reached threshold
        if (_language_count < RDS_THRESHOLD) {
          uint8_t language = rdsResponse.raw[Block_C_L];
          // Check if datum changed
          if (rds.language != language) {
            _language_count = 0;
            new_info = true;
          }
          // Save new data
          rds.language = language;
          ++_language_count;
        }
        break;
      }
    }
    // Groups 2A & 2B - Radio Text
    else if (type == 2) {
      // Check A/B flag to see if Radio Text has changed
      uint8_t new_ab = (bool)(rdsResponse.raw[Block_B_L] & 0b00010000);
      if (new_ab != _abRadioText) {
        // New message found - clear buffer
        _abRadioText = new_ab;
        for (uint8_t i = 0; i < sizeof(rds.radioText) - 1; i++)
          rds.radioText[i] = ' ';
        rds.radioTextLen = sizeof(rds.radioText); // Default to max length
      }
      // Get segment number
      segment = rdsResponse.raw[Block_B_L] & 0x0F;

      // Get Radio Text
      char *rt;       // Next position in rds.radioText[]
      uint8_t *block; // Next char from segment
      uint8_t i;      // Loop counter
      // TODO maybe: convert RDS non ASCII chars to UTF-8 for terminal interface
      if (version == 0) { // 2A
        rt = &rds.radioText[segment * 4];
        block = &rdsResponse.raw[Block_C_H];
        i = 4;
      } else { // 2B
        rt = &rds.radioText[segment * 2];
        block = &rdsResponse.raw[Block_D_H];
        i = 2;
      }
      // Copy chars
      do {
        // Get next char from segment
        char ch = *block++;
        // Check for end of message marker
        if (ch == '\r') {
          // Save new message length
          rds.radioTextLen = rt - rds.radioText;
        }
        // Put next char in rds.radioText[]
        *rt++ = make_printable(ch);
      } while (--i);
      new_info = true;
    }
    // Group 4A - Clock-time and date
    else if (type == 4 && version == 0) {
      // Only use if received perfectly.
      /* Note: Error Correcting Codes (ECC) are not perfect.  It is possible
       * for a block to be damaged enough that the ECC thinks the data is OK
       * when it's damaged or that it can recover when it cannot.  Because
       * date and time are useless unless accurate, we require that the date
       * and time be received perfectly to increase the odds of accurate data.
       */
      if ((rdsResponse.raw[12] &
           (FIELD_RDS_STATUS_RESP12_BLOCK_B | FIELD_RDS_STATUS_RESP12_BLOCK_C |
            FIELD_RDS_STATUS_RESP12_BLOCK_D)) ==
          (RDS_STATUS_RESP12_BLOCK_B_NO_ERRORS |
           RDS_STATUS_RESP12_BLOCK_C_NO_ERRORS |
           RDS_STATUS_RESP12_BLOCK_D_NO_ERRORS)) {
        // Get Modified Julian Date (MJD)
        rds.MJD = (rdsResponse.raw[Block_B_L] & 0b00000011) << 15UL |
                  rdsResponse.raw[Block_C_H] << 7U |
                  rdsResponse.raw[Block_C_L] >> 1U;

        // Get hour and minute
        rds.hour = (rdsResponse.raw[Block_C_L] & 0b00000001) << 4U |
                   rdsResponse.raw[Block_D_H] >> 4U;
        rds.minute = (rdsResponse.raw[Block_D_H] & 0x0F) << 2U |
                     rdsResponse.raw[Block_D_L] >> 6U;

        // Check if date and time sent (not 0)
        if (rds.MJD || rds.hour || rds.minute || rdsResponse.raw[Block_D_L]) {
          // Get offset to convert UTC to local time
          rds.offset = rdsResponse.raw[Block_D_L] & 0x1F;
          // Check if offset should be negative
          if (rdsResponse.raw[Block_D_L] & 0b00100000) {
            rds.offset = -rds.offset; // Make it negative
          }
          new_info = true;
        }
      }
    }
    // Group 10A - Program Type Name
    else if (type == 10 && version == 0) {
      // Check A/B flag to see if Program Type Name has changed
      uint8_t new_ab = (bool)(rdsResponse.raw[Block_B_L] & 0b00010000);
      if (new_ab != _abProgramTypeName) {
        // New name found - clear buffer
        _abProgramTypeName = new_ab;
        for (uint8_t i = 0; i < sizeof(rds.programTypeName) - 1; i++)
          rds.programTypeName[i] = ' ';
      }
      // Get segment number
      segment = rdsResponse.raw[Block_B_L] & 0x01;

      // Get Program Type Name
      char *name = &rds.programTypeName[segment * 4];
      *name++ = make_printable(rdsResponse.raw[Block_C_H]);
      *name++ = make_printable(rdsResponse.raw[Block_C_L]);
      *name++ = make_printable(rdsResponse.raw[Block_D_H]);
      *name = make_printable(rdsResponse.raw[Block_D_L]);
      new_info = true;
    }
  }
  return new_info;
}

#define DAYS_PER_YEAR 365U
// Leap year
#define DAYS_PER_LEAP_YEAR (DAYS_PER_YEAR + 1)
// Leap year every 4 years
#define DAYS_PER_4YEARS (DAYS_PER_YEAR * 4 + 1)
// Leap year every 4 years except century year (divisable by 100)
#define DAYS_PER_100YEARS (DAYS_PER_4YEARS * (100 / 4) - 1)

// Get last RDS date and time converted to local date and time.
// Returns true if current station has sent date and time.  Otherwise, it
// returns false and writes nothing to structure. Only provides info if mode==FM
// and station is sending RDS data.
bool SI4732_GetLocalDateTime(DateTime *time) {
  // Look for date/time info
  if (rds.offset == NO_DATE_TIME)
    return false; // No date or time info available

  // Origin for Modified Julian Date (MJD) is November 17, 1858, Wednesday.
  // Move origin to Jan. 2, 2000, Sunday.
  // Note: We don't use Jan. 1 to compensate for the fact that 2000 is a leap
  // year.
  unsigned short days = rds.MJD - (                          // 1858-Nov-17
                                      14 +                   // 1858-Dec-1
                                      31 +                   // 1859-Jan-1
                                      DAYS_PER_YEAR +        // 1860-Jan-1
                                      10 * DAYS_PER_4YEARS + // 1900-Jan-1
                                      DAYS_PER_100YEARS +    // 2000-Jan-1
                                      1);                    // 2000-Jan-2

  // Convert UTC date and time to local date and time.
  // Combine date and time
  unsigned long date_time = ((unsigned long)days) * (24 * 60) +
                            ((unsigned short)rds.hour) * 60 + rds.minute;
  // Adjust offset from units of half hours to minutes
  int16_t offset = (int16_t)(rds.offset) * 30;
  // Compute local date/time
  date_time += offset;
  // Break down date and time
  time->minute = date_time % 60;
  date_time /= 60;
  time->hour = date_time % 24;
  days = date_time / 24;

  // Compute day of the week - Sunday = 0
  time->wday = days % 7;

  // Compute year
  unsigned char leap_year = 0; /* 1 if leap year, else 0 */
  // Note: This code assumes all century years (2000, 2100...) are not leap
  // years. This will break in 2400 AD.  However, RDS' date field will overflow
  // long before 2400 AD.
  time->year = days / DAYS_PER_100YEARS * 100 + 2000;
  days %= DAYS_PER_100YEARS;
  if (!(days < DAYS_PER_YEAR)) {
    days++; // Adjust for no leap year for century year
    time->year += days / DAYS_PER_4YEARS * 4;
    days %= DAYS_PER_4YEARS;
    if (days < DAYS_PER_LEAP_YEAR) {
      leap_year = 1;
    } else {
      days--; // Adjust for leap year for first of 4 years
      time->year += days / DAYS_PER_YEAR;
      days %= DAYS_PER_YEAR;
    }
  }

  // Compute month and day of the month
  if (days < 31 + 28 + leap_year) {
    if (days < 31) {
      /* January */
      time->month = 1;
      time->day = days + 1;
    } else {
      /* February */
      time->month = 2;
      time->day = days + 1 - 31;
    }
  } else {
    /* March - December */
    enum { NUM_MONTHS = 10 };
    static const unsigned short month[NUM_MONTHS] = {
        0,
        31,
        31 + 30,
        31 + 30 + 31,
        31 + 30 + 31 + 30,
        31 + 30 + 31 + 30 + 31,
        31 + 30 + 31 + 30 + 31 + 31,
        31 + 30 + 31 + 30 + 31 + 31 + 30,
        31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
        31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30};
    unsigned short value; // Value from table
    unsigned char mon;    // Index to month[]

    days -= 31 + 28 + leap_year;
    // Look up month
    for (mon = NUM_MONTHS; days < (value = (month[--mon]));)
      ;
    time->day = days - value + 1;
    time->month = mon + 2 + 1;
  }
  return true;
}

bool SI4732_GetLocalTime(Time *time) {
  // Look for date/time info
  if (rds.offset == NO_DATE_TIME)
    return false; // No date or time info available

  // Convert UTC to local time
  /* Note: If the offset is negative, 'hour' and 'minute' could become negative.
   * To compensate, we add 24 to hour and 60 to minute.  We then do a modulus
   * division (%24 and %60) to correct for any overflow caused by either a
   * positive offset or the above mentioned addition.
   */
  time->hour = (rds.hour + rds.offset / 2 + 24) % 24;
  time->minute = (rds.minute + rds.offset % 2 * 30 + 60) % 60;
  return true;
}

void SI4732_GetProgramType(char buffer[17]) {
    static const char PTY_RBDS_to_str[51][16]={
      ' ',' ',' ',' ',' ',' ','N','o','n','e',' ',' ',' ',' ',' ',' ',
      ' ',' ',' ',' ',' ',' ','N','e','w','s',' ',' ',' ',' ',' ',' ',
      ' ',' ','I','n','f','o','r','m','a','t','i','o','n',' ',' ',' ',
      ' ',' ',' ',' ',' ','S','p','o','r','t','s',' ',' ',' ',' ',' ',
      ' ',' ',' ',' ',' ',' ','T','a','l','k',' ',' ',' ',' ',' ',' ',
      ' ',' ',' ',' ',' ',' ','R','o','c','k',' ',' ',' ',' ',' ',' ',
      ' ',' ','C','l','a','s','s','i','c',' ','R','o','c','k',' ',' ',
      ' ',' ',' ','A','d','u','l','t',' ','H','i','t','s',' ',' ',' ',
      ' ',' ',' ','S','o','f','t',' ','R','o','c','k',' ',' ',' ',' ',
      ' ',' ',' ',' ',' ','T','o','p',' ','4','0',' ',' ',' ',' ',' ',
      ' ',' ',' ',' ','C','o','u','n','t','r','y',' ',' ',' ',' ',' ',
      ' ',' ',' ',' ',' ','O','l','d','i','e','s',' ',' ',' ',' ',' ',
      ' ',' ',' ',' ',' ',' ','S','o','f','t',' ',' ',' ',' ',' ',' ',
      ' ',' ',' ','N','o','s','t','a','l','g','i','a',' ',' ',' ',' ',
      ' ',' ',' ',' ',' ',' ','J','a','z','z',' ',' ',' ',' ',' ',' ',
      ' ',' ',' ','C','l','a','s','s','i','c','a','l',' ',' ',' ',' ',
      'R','h','y','t','h','m',' ','a','n','d',' ','B','l','u','e','s',
      ' ',' ',' ','S','o','f','t',' ','R',' ','&',' ','B',' ',' ',' ',
      'F','o','r','e','i','g','n',' ','L','a','n','g','u','a','g','e',
      'R','e','l','i','g','i','o','u','s',' ','M','u','s','i','c',' ',
      ' ','R','e','l','i','g','i','o','u','s',' ','T','a','l','k',' ',
      ' ',' ','P','e','r','s','o','n','a','l','i','t','y',' ',' ',' ',
      ' ',' ',' ',' ',' ','P','u','b','l','i','c',' ',' ',' ',' ',' ',
      ' ',' ',' ',' ','C','o','l','l','e','g','e',' ',' ',' ',' ',' ',
      ' ',' ','S','p','a','n','i','s','h',' ','T','a','l','k',' ',' ',
      ' ','S','p','a','n','i','s','h',' ','M','u','s','i','c',' ',' ',
      ' ',' ',' ',' ','H','i','p',' ','H','o','p',' ',' ',' ',' ',' ',
      ' ','R','e','s','e','r','v','e','d',' ',' ','-','2','7','-',' ',
      ' ','R','e','s','e','r','v','e','d',' ',' ','-','2','8','-',' ',
      ' ',' ',' ',' ',' ','W','e','a','t','h','e','r',' ',' ',' ',' ',
      ' ','E','m','e','r','g','e','n','c','y',' ','T','e','s','t',' ',
      ' ','A','L','E','R','T','!',' ','A','L','E','R','T','!',' ',' ',
      //Following messages are for locales outside USA (RDS)
      'C','u','r','r','e','n','t',' ','A','f','f','a','i','r','s',' ',
      ' ',' ',' ','E','d','u','c','a','t','i','o','n',' ',' ',' ',' ',
      ' ',' ',' ',' ',' ','D','r','a','m','a',' ',' ',' ',' ',' ',' ',
      ' ',' ',' ',' ','C','u','l','t','u','r','e','s',' ',' ',' ',' ',
      ' ',' ',' ',' ','S','c','i','e','n','c','e',' ',' ',' ',' ',' ',
      ' ','V','a','r','i','e','d',' ','S','p','e','e','c','h',' ',' ',
      ' ','E','a','s','y',' ','L','i','s','t','e','n','i','n','g',' ',
      ' ','L','i','g','h','t',' ','C','l','a','s','s','i','c','s',' ',
      'S','e','r','i','o','u','s',' ','C','l','a','s','s','i','c','s',
      ' ',' ','O','t','h','e','r',' ','M','u','s','i','c',' ',' ',' ',
      ' ',' ',' ',' ','F','i','n','a','n','c','e',' ',' ',' ',' ',' ',
      'C','h','i','l','d','r','e','n','\'','s',' ','P','r','o','g','s',
      ' ','S','o','c','i','a','l',' ','A','f','f','a','i','r','s',' ',
      ' ',' ',' ',' ','P','h','o','n','e',' ','I','n',' ',' ',' ',' ',
      'T','r','a','v','e','l',' ','&',' ','T','o','u','r','i','n','g',
      'L','e','i','s','u','r','e',' ','&',' ','H','o','b','b','y',' ',
      ' ','N','a','t','i','o','n','a','l',' ','M','u','s','i','c',' ',
      ' ',' ',' ','F','o','l','k',' ','M','u','s','i','c',' ',' ',' ',
      ' ',' ','D','o','c','u','m','e','n','t','a','r','y',' ',' ',' '
   };

    const char *str;  //String from PTY_RBDS_to_str[] array.

   //Translate PTY code into English text based on RBDS/RDS flag.
   if(rds.RBDS){
      str = PTY_RBDS_to_str[rds.programType];
   }else{
      //Translate RDS PTY code to RBDS PTY code
      //Note: Codes above 31 do not actually exist but can be used with the PTY_RBDS_to_str[] table.
      static const uint8_t PTY_RDS_to_RBDS[32]={
         0, 1, 32, 2,
         3, 33, 34, 35,
         36, 37, 9, 5,
         38, 39, 40, 41,
         29, 42, 43, 44,
         20, 45, 46, 47,
         14, 10, 48, 11,
         49, 50, 30, 31
      };
      str = PTY_RBDS_to_str[PTY_RDS_to_RBDS[rds.programType]];
   }
   //Copy text to caller's buffer.
   strncpy(buffer, str, 16);
   buffer[16]='\0';
}

void SI4732_SwitchMode() { SI4732_PowerDown(); }

void SI4732_SetFreq(uint32_t freq) {
  waitToSend();
  currentFreq = freq;
  uint8_t hb = (freq >> 8) & 0xFF;
  uint8_t lb = freq & 0xFF;
  if (si4732mode == SI4732_FM) {
    uint8_t cmd[5] = {SI4735_CMD_FM_TUNE_FREQ, 0x00, hb, lb};
    SI4732_WriteBuffer(cmd, 4);

  } else if (si4732mode == SI4732_AM) {
    uint8_t cmd[6] = {0x40, 0x00, hb, lb, 0};
    SI4732_WriteBuffer(cmd, 5);
  }
  waitToSend();

  SYSTEM_DelayMs(30);
  //    SYSTEM_DelayMs(500);
  RSQ_GET();
}

void SI4732_GET_INT_STATUS() {
  uint8_t state = 0;
  while (state != 0x81) {
    uint8_t cmd3[1] = {0x14};
    SI4732_WriteBuffer(cmd3, 1);
    SI4732_ReadBuffer((uint8_t *)&state, 1);
  }
  SYSTEM_DelayMs(SI4732_DELAY_MS);
}

void setVolume(uint8_t volume) {
#define RX_VOLUME 0x4000
  if (volume < 0)
    volume = 0;
  if (volume > 63)
    volume = 63;
  sendProperty(RX_VOLUME, volume);
}

void setAvcAmMaxGain(uint8_t gain) {
  if (gain < 12 || gain > 90)
    return;
  sendProperty(0x3103, gain * 340);
}

void AM_FRONTEND_AGC_CONTROL(uint8_t MIN_GAIN_INDEX, uint8_t ATTN_BACKUP) {
  uint16_t num = MIN_GAIN_INDEX << 8 | ATTN_BACKUP;
  sendProperty(0x3705, num);
}

void setAmSoftMuteMaxAttenuation(uint8_t smattn) {
  sendProperty(0x3302, smattn);
}

void setBandwidth(uint8_t AMCHFLT, uint8_t AMPLFLT) {
  waitToSend();

  uint8_t tmp[6] = {0x12, 0, 0x31, 0x02, AMCHFLT, AMPLFLT};
  SI4732_WriteBuffer(tmp, 6);

  waitToSend();
}

void SI4732_Init() {
  /* SYSTEM_DelayMs(SI4732_DELAY_MS);

  SI4732_PowerDown();
  SYSTEM_DelayMs(SI4732_DELAY_MS);

  AUDIO_ToggleSpeaker(false); */

  RST_LOW;
  SYSTEM_DelayMs(10);
  RST_HIGH;
  SYSTEM_DelayMs(10);

  SI4732_PowerUp();

  AUDIO_ToggleSpeaker(true);
  setVolume(63); // Default volume level.

  if (si4732mode == SI4732_AM) {
    SYSTEM_DelayMs(250);
    setAvcAmMaxGain(48);
    SYSTEM_DelayMs(500);

    waitToSend();
    uint8_t cmd2[6] = {SI4735_CMD_AM_AGC_OVERRIDE, 0x01, 0};
    SI4732_WriteBuffer(cmd2, 3);
    waitToSend();

    setAmSoftMuteMaxAttenuation(8);

    setBandwidth(2, 1);

    setAvcAmMaxGain(38);

    AM_FRONTEND_AGC_CONTROL(10, 12);
  } else {
    /* waitToSend();
    uint8_t cmd2[6] = {SI4735_CMD_FM_AGC_OVERRIDE, 0x01, 0};
    SI4732_WriteBuffer(cmd2, 3);
    SYSTEM_DelayMs(100); */
  }

  SI4732_SetFreq(currentFreq);

  SI4732_GET_INT_STATUS();

  SYSTEM_DelayMs(100);
}
