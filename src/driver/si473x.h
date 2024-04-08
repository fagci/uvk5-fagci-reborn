#ifndef SI473X_H
#define SI473X_H

#include <stdint.h>

#define SI4735_CMD_POWER_UP 0x01
#define SI4735_CMD_GET_REV 0x10
#define SI4735_CMD_POWER_DOWN 0x11
#define SI4735_CMD_SET_PROPERTY 0x12
#define SI4735_CMD_GET_PROPERTY 0x13
#define SI4735_CMD_GET_INT_STATUS 0x14
#define SI4735_CMD_PATCH_ARGS 0x15
#define SI4735_CMD_PATCH_DATA 0x16
#define SI4735_CMD_FM_TUNE_FREQ 0x20
#define SI4735_CMD_FM_SEEK_START 0x21
#define SI4735_CMD_FM_TUNE_STATUS 0x22
#define SI4735_CMD_FM_RSQ_STATUS 0x23
#define SI4735_CMD_FM_RDS_STATUS 0x24
#define SI4735_CMD_FM_AGC_STATUS 0x27
#define SI4735_CMD_FM_AGC_OVERRIDE 0x28
#define SI4735_CMD_TX_TUNE_FREQ 0x30
#define SI4735_CMD_TX_TUNE_POWER 0x31
#define SI4735_CMD_TX_TUNE_MEASURE 0x32
#define SI4735_CMD_TX_TUNE_STATUS 0x33
#define SI4735_CMD_TX_ASQ_STATUS 0x34
#define SI4735_CMD_TX_RDS_BUF 0x35
#define SI4735_CMD_TX_RDS_PS 0x36
#define SI4735_CMD_AM_TUNE_FREQ 0x40
#define SI4735_CMD_AM_SEEK_START 0x41
#define SI4735_CMD_AM_TUNE_STATUS 0x42
#define SI4735_CMD_AM_RSQ_STATUS 0x43
#define SI4735_CMD_AM_AGC_STATUS 0x47
#define SI4735_CMD_AM_AGC_OVERRIDE 0x48
#define SI4735_CMD_WB_TUNE_FREQ 0x50
#define SI4735_CMD_WB_TUNE_STATUS 0x52
#define SI4735_CMD_WB_RSQ_STATUS 0x53
#define SI4735_CMD_WB_SAME_STATUS 0x54
#define SI4735_CMD_WB_ASQ_STATUS 0x55
#define SI4735_CMD_WB_AGC_STATUS 0x57
#define SI4735_CMD_WB_AGC_OVERRIDE 0x58
#define SI4735_CMD_AUX_ASRC_START 0x61
#define SI4735_CMD_AUX_ASQ_STATUS 0x65
#define SI4735_CMD_GPIO_CTL 0x80
#define SI4735_CMD_GPIO_SET 0x81

#define SI4735_FLG_CTSIEN 0x80
// Renamed to GPO2IEN from GPO2OEN in datasheet to avoid conflict with real
// GPO2OEN below. Also makes more sense this way: GPO2IEN -> enable GPO2 as INT
// GPO2OEN -> enable GPO2 generically, as an output
#define SI4735_FLG_GPO2IEN 0x40
#define SI4735_FLG_PATCH 0x20
#define SI4735_FLG_XOSCEN 0x10
#define SI4735_FLG_FREEZE 0x02
#define SI4735_FLG_FAST 0x01
#define SI4735_FLG_SEEKUP 0x08
#define SI4735_FLG_WRAP 0x04
#define SI4735_FLG_CANCEL 0x02
#define SI4735_FLG_INTACK 0x01
#define SI4735_FLG_STATUSONLY 0x04
#define SI4735_FLG_MTFIFO 0x02
#define SI4735_FLG_GPO3OEN 0x08
#define SI4735_FLG_GPO2OEN 0x04
#define SI4735_FLG_GPO1OEN 0x02
#define SI4735_FLG_GPO3LEVEL 0x08
#define SI4735_FLG_GPO2LEVEL 0x04
#define SI4735_FLG_GPO1LEVEL 0x02
#define SI4735_FLG_BLETHA_0 0x00
#define SI4735_FLG_BLETHA_12 0x40
#define SI4735_FLG_BLETHA_35 0x80
#define SI4735_FLG_BLETHA_U (SI4735_FLG_BLETHA_12 | SI4735_FLG_BLETHA_35)
#define SI4735_FLG_BLETHB_0 SI4735_FLG_BLETHA_0
#define SI4735_FLG_BLETHB_12 0x10
#define SI4735_FLG_BLETHB_35 0x20
#define SI4735_FLG_BLETHB_U (SI4735_FLG_BLETHB_12 | SI4735_FLG_BLETHB_35)
#define SI4735_FLG_BLETHC_0 SI4735_FLG_BLETHA_0
#define SI4735_FLG_BLETHC_12 0x04
#define SI4735_FLG_BLETHC_35 0x08
#define SI4735_FLG_BLETHC_U (SI4735_FLG_BLETHC_12 | SI4735_FLG_BLETHC_35)
#define SI4735_FLG_BLETHD_0 SI4735_FLG_BLETHA_0
#define SI4735_FLG_BLETHD_12 0x01
#define SI4735_FLG_BLETHD_35 0x02
#define SI4735_FLG_BLETHD_U (SI4735_FLG_BLETHD_12 | SI4735_FLG_BLETHD_35)
#define SI4735_FLG_RDSEN 0x01
#define SI4735_FLG_DEEMPH_NONE 0x00
#define SI4735_FLG_DEEMPH_50 0x01
#define SI4735_FLG_DEEMPH_75 0x02
#define SI4735_FLG_RSQREP 0x08
#define SI4735_FLG_RDSREP 0x04
#define SI4735_FLG_STCREP 0x01
#define SI4735_FLG_ERRIEN 0x40
#define SI4735_FLG_RSQIEN 0x08
#define SI4735_FLG_RDSIEN 0x04
#define SI4735_FLG_STCIEN 0x01
#define SI4735_FLG_RDSNEWBLOCKB 0x20
#define SI4735_FLG_RDSNEWBLOCKA 0x10
#define SI4735_FLG_RDSSYNCFOUND 0x04
#define SI4735_FLG_RDSSYNCLOST 0x02
#define SI4735_FLG_RDSRECV 0x01
#define SI4735_FLG_GRPLOST 0x04
#define SI4735_FLG_RDSSYNC 0x01
#define SI4735_FLG_AMPLFLT 0x01
#define SI4735_FLG_AMCHFLT_6KHZ 0x00
#define SI4735_FLG_AMCHFLT_4KHZ 0x01
#define SI4735_FLG_AMCHFLT_3KHZ 0x02
#define SI4735_FLG_AMCHFLT_2KHZ 0x03
#define SI4735_FLG_AMCHFLT_1KHZ 0x04
#define SI4735_FLG_AMCHFLT_1KHZ8 0x05
#define SI4735_FLG_AMCHFLT_2KHZ5 0x06

// Define Si4735 Function modes
#define SI4735_FUNC_FM 0x00
#define SI4735_FUNC_AM 0x01
#define SI4735_FUNC_VER 0x0F

// Define Si4735 Output modes
#define SI4735_OUT_RDS 0x00 // RDS only
#define SI4735_OUT_ANALOG 0x05
#define SI4735_OUT_DIGITAL1 0x0B // DCLK, LOUT/DFS, ROUT/DIO
#define SI4735_OUT_DIGITAL2 0xB0 // DCLK, DFS, DIO
#define SI4735_OUT_BOTH (SI4735_OUT_ANALOG | SI4735_OUT_DIGITAL2)

// Define Si47xx Status flag masks (bits the chip fed us)
#define SI4735_STATUS_CTS 0x80
#define SI4735_STATUS_ERR 0x40
#define SI4735_STATUS_RSQINT 0x08
#define SI4735_STATUS_RDSINT 0x04
#define SI4735_STATUS_ASQINT 0x02
#define SI4735_STATUS_STCINT 0x01
#define SI4735_STATUS_BLTF 0x80
#define SI4735_STATUS_AFCRL 0x02
#define SI4735_STATUS_VALID 0x01
#define SI4735_STATUS_BLENDINT 0x80
#define SI4735_STATUS_MULTHINT 0x20
#define SI4735_STATUS_MULTLINT 0x10
#define SI4735_STATUS_SNRHINT 0x08
#define SI4735_STATUS_SNRLINT 0x04
#define SI4735_STATUS_RSSIHINT 0x02
#define SI4735_STATUS_RSSILINT 0x01
#define SI4735_STATUS_SMUTE 0x08
#define SI4735_STATUS_PILOT 0x80
#define SI4735_STATUS_OVERMOD 0x04
#define SI4735_STATUS_IALH 0x02
#define SI4735_STATUS_IALL 0x01

// Define Si47xx Property codes
#define SI4735_PROP_GPO_IEN word(0x0001)
#define SI4735_PROP_DIGITAL_INPUT_FORMAT 0x0101
#define SI4735_PROP_DIGITAL_OUTPUT_FORMAT 0x0102
#define SI4735_PROP_DIGITAL_INPUT_SAMPLE_RATE 0x0103
#define SI4735_PROP_DIGITAL_OUTPUT_SAMPLE_RATE 0x0104
#define SI4735_PROP_REFCLK_FREQ 0x0201
#define SI4735_PROP_REFCLK_PRESCALE 0x0202
#define SI4735_PROP_FM_DEEMPHASIS 0x1100
#define SI4735_PROP_FM_CHANNEL_FILTER 0x1102
#define SI4735_PROP_FM_BLEND_STEREO_THRESHOLD 0x1105
#define SI4735_PROP_FM_BLEND_MONO_THRESHOLD 0x1106
#define SI4735_PROP_FM_ANTENNA_INPUT 0x1107
#define SI4735_PROP_FM_MAX_TUNE_ERROR 0x1108
#define SI4735_PROP_FM_RSQ_INT_SOURCE 0x1200
#define SI4735_PROP_FM_RSQ_SNR_HI_THRESHOLD 0x1201
#define SI4735_PROP_FM_RSQ_SNR_LO_THRESHOLD 0x1202
#define SI4735_PROP_FM_RSQ_RSSI_HI_THRESHOLD 0x1203
#define SI4735_PROP_FM_RSQ_RSSI_LO_THRESHOLD 0x1204
#define SI4735_PROP_FM_RSQ_MULTIPATH_HI_THRESHOLD 0x1205
#define SI4735_PROP_FM_RSQ_MULTIPATH_LO_THRESHOLD 0x1206
#define SI4735_PROP_FM_RSQ_BLEND_THRESHOLD 0x1207
#define SI4735_PROP_FM_SOFT_MUTE_RATE 0x1300
#define SI4735_PROP_FM_SOFT_MUTE_SLOPE 0x1301
#define SI4735_PROP_FM_SOFT_MUTE_MAX_ATTENUATION 0x1302
#define SI4735_PROP_FM_SOFT_MUTE_SNR_THRESHOLD 0x1303
#define SI4735_PROP_FM_SOFT_MUTE_RELEASE_RATE 0x1304
#define SI4735_PROP_FM_SOFT_MUTE_ATTACK_RATE 0x1305
#define SI4735_PROP_FM_SEEK_BAND_BOTTOM 0x1400
#define SI4735_PROP_FM_SEEK_BAND_TOP 0x1401
#define SI4735_PROP_FM_SEEK_FREQ_SPACING 0x1402
#define SI4735_PROP_FM_SEEK_TUNE_SNR_THRESHOLD 0x1403
#define SI4735_PROP_FM_SEEK_TUNE_RSSI_THRESHOLD 0x1404
#define SI4735_PROP_FM_RDS_INT_SOURCE 0x1500
#define SI4735_PROP_FM_RDS_INT_FIFO_COUNT 0x1501
#define SI4735_PROP_FM_RDS_CONFIG 0x1502
#define SI4735_PROP_FM_RDS_CONFIDENCE 0x1503
#define SI4735_PROP_FM_AGC_ATTACK_RATE 0x1700
#define SI4735_PROP_FM_AGC_RELEASE_RATE 0x1701
#define SI4735_PROP_FM_BLEND_RSSI_STEREO_THRESHOLD 0x1800
#define SI4735_PROP_FM_BLEND_RSSI_MONO_THRESHOLD 0x1801
#define SI4735_PROP_FM_BLEND_RSSI_ATTACK_RATE 0x1802
#define SI4735_PROP_FM_BLEND_RSSI_RELEASE_RATE 0x1803
#define SI4735_PROP_FM_BLEND_SNR_STEREO_THRESHOLD 0x1804
#define SI4735_PROP_FM_BLEND_SNR_MONO_THRESHOLD 0x1805
#define SI4735_PROP_FM_BLEND_SNR_ATTACK_RATE 0x1806
#define SI4735_PROP_FM_BLEND_SNR_RELEASE_RATE 0x1807
#define SI4735_PROP_FM_BLEND_MULTIPATH_STEREO_THRESHOLD 0x1808
#define SI4735_PROP_FM_BLEND_MULTIPATH_MONO_THRESHOLD 0x1809
#define SI4735_PROP_FM_BLEND_MULTIPATH_ATTACK_RATE 0x180A
#define SI4735_PROP_FM_BLEND_MULTIPATH_RELEASE_RATE 0x180B
#define SI4735_PROP_FM_BLEND_MAX_STEREO_SEPARATION 0x180C
#define SI4735_PROP_FM_NB_DETECT_THRESHOLD 0x1900
#define SI4735_PROP_FM_NB_INTERVAL 0x1901
#define SI4735_PROP_FM_NB_RATE 0x1902
#define SI4735_PROP_FM_NB_IIR_FILTER 0x1903
#define SI4735_PROP_FM_NB_DELAY 0x1904
#define SI4735_PROP_FM_HICUT_SNR_HIGH_THRESHOLD 0x1A00
#define SI4735_PROP_FM_HICUT_SNR_LOW_THRESHOLD 0x1A01
#define SI4735_PROP_FM_HICUT_ATTACK_RATE 0x1A02
#define SI4735_PROP_FM_HICUT_RELEASE_RATE 0x1A03
#define SI4735_PROP_FM_HICUT_MULTIPATH_TRIGGER_THRESHOLD 0x1A04
#define SI4735_PROP_FM_HICUT_MULTIPATH_END_THRESHOLD 0x1A05
#define SI4735_PROP_FM_HICUT_CUTOFF_FREQUENCY 0x1A06
#define SI4735_PROP_TX_COMPONENT_ENABLE 0x2100
#define SI4735_PROP_TX_AUDIO_DEVIATION 0x2101
#define SI4735_PROP_TX_PILOT_DEVIATION 0x2102
#define SI4735_PROP_TX_RDS_DEVIATION 0x2103
#define SI4735_PROP_TX_LINE_INPUT_LEVEL 0x2104
#define SI4735_PROP_TX_LINE_INPUT_MUTE 0x2105
#define SI4735_PROP_TX_PREEMPHASIS 0x2106
#define SI4735_PROP_TX_PILOT_FREQUENCY 0x2107
#define SI4735_PROP_TX_ACOMP_ENABLE 0x2200
#define SI4735_PROP_TX_ACOMP_THRESHOLD 0x2201
#define SI4735_PROP_TX_ACOMP_ATTACK_TIME 0x2202
#define SI4735_PROP_TX_ACOMP_RELEASE_TIME 0x2203
#define SI4735_PROP_TX_ACOMP_GAIN 0x2204
#define SI4735_PROP_TX_LIMITER_RELEASE_TIME 0x2205
#define SI4735_PROP_TX_ASQ_INTERRUPT_SOURCE 0x2300
#define SI4735_PROP_TX_ASQ_LEVEL_LOW 0x2301
#define SI4735_PROP_TX_ASQ_DURATION_LOW 0x2302
#define SI4735_PROP_TX_ASQ_LEVEL_HIGH 0x2303
#define SI4735_PROP_TX_ASQ_DURATION_HIGH 0x2304
#define SI4735_PROP_TX_RDS_INTERRUPT_SOURCE 0x2C00
#define SI4735_PROP_TX_RDS_PI 0x2C01
#define SI4735_PROP_TX_RDS_PS_MIX 0x2C02
#define SI4735_PROP_TX_RDS_PS_MISC 0x2C03
#define SI4735_PROP_TX_RDS_PS_REPEAT_COUNT 0x2C04
#define SI4735_PROP_TX_RDS_PS_MESSAGE_COUNT 0x2C05
#define SI4735_PROP_TX_RDS_PS_AF 0x2C06
#define SI4735_PROP_TX_RDS_FIFO_SIZE 0x2C07
#define SI4735_PROP_AM_DEEMPHASIS 0x3100
#define SI4735_PROP_AM_CHANNEL_FILTER 0x3102
#define SI4735_PROP_AM_AUTOMATIC_VOLUME_CONTROL_MAX_GAIN 0x3103
#define SI4735_PROP_AM_MODE_AFC_SW_PULL_IN_RANGE 0x3104
#define SI4735_PROP_AM_MODE_AFC_SW_LOCK_IN_RANGE 0x3105
#define SI4735_PROP_AM_RSQ_INTERRUPTS 0x3200
#define SI4735_PROP_AM_RSQ_SNR_HIGH_THRESHOLD 0x3201
#define SI4735_PROP_AM_RSQ_SNR_LOW_THRESHOLD 0x3202
#define SI4735_PROP_AM_RSQ_RSSI_HIGH_THRESHOLD 0x3203
#define SI4735_PROP_AM_RSQ_RSSI_LOW_THRESHOLD 0x3204
#define SI4735_PROP_AM_SOFT_MUTE_RATE 0x3300
#define SI4735_PROP_AM_SOFT_MUTE_SLOPE 0x3301
#define SI4735_PROP_AM_SOFT_MUTE_MAX_ATTENUATION 0x3302
#define SI4735_PROP_AM_SOFT_MUTE_SNR_THRESHOLD 0x3303
#define SI4735_PROP_AM_SOFT_MUTE_RELEASE_RATE 0x3304
#define SI4735_PROP_AM_SOFT_MUTE_ATTACK_RATE 0x3305
#define SI4735_PROP_AM_SEEK_BAND_BOTTOM 0x3400
#define SI4735_PROP_AM_SEEK_BAND_TOP 0x3401
#define SI4735_PROP_AM_SEEK_FREQ_SPACING 0x3402
#define SI4735_PROP_AM_SEEK_TUNE_SNR_THRESHOLD 0x3403
#define SI4735_PROP_AM_SEEK_TUNE_RSSI_THRESHOLD 0x3404
#define SI4735_PROP_AM_AGC_ATTACK_RATE 0x3702
#define SI4735_PROP_AM_AGC_RELEASE_RATE 0x3703
#define SI4735_PROP_AM_FRONTEND_AGC_CONTROL 0x3705
#define SI4735_PROP_AM_NB_DETECT_THRESHOLD 0x3900
#define SI4735_PROP_AM_NB_INTERVAL 0x3901
#define SI4735_PROP_AM_NB_RATE 0x3902
#define SI4735_PROP_AM_NB_IIR_FILTER 0x3903
#define SI4735_PROP_AM_NB_DELAY 0x3904
#define SI4735_PROP_RX_VOLUME 0x4000
#define SI4735_PROP_RX_HARD_MUTE 0x4001
#define SI4735_PROP_WB_MAX_TUNE_ERROR 0x5108
#define SI4735_PROP_WB_RSQ_INT_SOURCE 0x5200
#define SI4735_PROP_WB_RSQ_SNR_HI_THRESHOLD 0x5201
#define SI4735_PROP_WB_RSQ_SNR_LO_THRESHOLD 0x5202
#define SI4735_PROP_WB_RSQ_RSSI_HI_THRESHOLD 0x5203
#define SI4735_PROP_WB_RSQ_RSSI_LO_THRESHOLD 0x5204
#define SI4735_PROP_WB_VALID_SNR_THRESHOLD 0x5403
#define SI4735_PROP_WB_VALID_RSSI_THRESHOLD 0x5404
#define SI4735_PROP_WB_SAME_INTERRUPT_SOURCE 0x5500
#define SI4735_PROP_WB_ASQ_INTERRUPT_SOURCE 0x5600
#define SI4735_PROP_AUX_ASQ_INTERRUPT_SOURCE 0x6600
#define SI4735_PROP_DEBUG_CONTROL 0xFF00

typedef enum { SI4732_FM, SI4732_AM } SI4732_MODE;
typedef union {
  struct {
    // status ("RESP0")
    uint8_t STCINT : 1;
    uint8_t DUMMY1 : 1;
    uint8_t RDSINT : 1;
    uint8_t RSQINT : 1;
    uint8_t DUMMY2 : 2;
    uint8_t ERR : 1;
    uint8_t CTS : 1;
    // RESP1
    uint8_t RDSRECV : 1; //!<  RDS Received; 1 = FIFO filled to minimum number
                         //!<  of groups set by RDSFIFOCNT.
    uint8_t RDSSYNCLOST : 1; //!<  RDS Sync Lost; 1 = Lost RDS synchronization.
    uint8_t
        RDSSYNCFOUND : 1; //!<  RDS Sync Found; 1 = Found RDS synchronization.
    uint8_t DUMMY3 : 1;
    uint8_t RDSNEWBLOCKA : 1; //!<  RDS New Block A; 1 = Valid Block A data has
                              //!<  been received.
    uint8_t RDSNEWBLOCKB : 1; //!<  RDS New Block B; 1 = Valid Block B data has
                              //!<  been received.
    uint8_t DUMMY4 : 2;
    // RESP2
    uint8_t RDSSYNC : 1; //!<  RDS Sync; 1 = RDS currently synchronized.
    uint8_t DUMMY5 : 1;
    uint8_t GRPLOST : 1; //!<  Group Lost; 1 = One or more RDS groups discarded
                         //!<  due to FIFO overrun.
    uint8_t DUMMY6 : 5;
    // RESP3 to RESP11
    uint8_t RDSFIFOUSED; //!<  RESP3 - RDS FIFO Used; Number of groups remaining
                         //!<  in the RDS FIFO (0 if empty).
    uint8_t BLOCKAH;     //!<  RESP4 - RDS Block A; HIGH byte
    uint8_t BLOCKAL;     //!<  RESP5 - RDS Block A; LOW byte
    uint8_t BLOCKBH;     //!<  RESP6 - RDS Block B; HIGH byte
    uint8_t BLOCKBL;     //!<  RESP7 - RDS Block B; LOW byte
    uint8_t BLOCKCH;     //!<  RESP8 - RDS Block C; HIGH byte
    uint8_t BLOCKCL;     //!<  RESP9 - RDS Block C; LOW byte
    uint8_t BLOCKDH;     //!<  RESP10 - RDS Block D; HIGH byte
    uint8_t BLOCKDL;     //!<  RESP11 - RDS Block D; LOW byte
    // RESP12 - Blocks A to D Corrected Errors.
    // 0 = No errors;
    // 1 = 1–2 bit errors detected and corrected;
    // 2 = 3–5 bit errors detected and corrected.
    // 3 = Uncorrectable.
    uint8_t BLED : 2;
    uint8_t BLEC : 2;
    uint8_t BLEB : 2;
    uint8_t BLEA : 2;
  } resp;
  uint8_t raw[13];
} si47x_rds_status;

typedef signed char ternary;
/* RDS and RBDS data */
typedef struct {
  uint16_t
      programId; // Program Identification (PI) code - unique code assigned to
                 // program. In the US, except for simulcast stations, each
                 // station has a unique PI. PI = 0 if no RDS info received.
  /* groupA and groupB indicate if the station has broadcast one or more of each
   * RDS group type and version. There is one bit for each group type.  Bit
   * number 0 is for group type 0, and so on. groupA gives version A groups
   * (packets), groupB gives version B groups. If a bit is true then one or more
   * of that group type and version has been received. Example:  If (groupA &
   * 1<<4) is true then at least one Group type 4, version A group (packet) has
   * been received. Note: If the RDS signal is weak, many bad packets will be
   * received.  Sometimes, the packets are so corrupted that the radio thinks
   * the bad data is OK.  This can cause false information to be recorded in the
   * groupA and groupB variables.
   */
  uint16_t groupA;     // One bit for each group type, version A
  uint16_t groupB;     // One bit for each group type, version B
  bool RDSSignal;      // True if RDS (or RBDS) signal currently detected
  bool RBDS;           // True if station using RBDS, else using RDS
  uint8_t programType; // Program Type (PTY) code - identifies program format -
                       // call getProgramTypeStr()
  uint8_t extendedCountryCode; // Extended Country Code (ECC) - constants
                               // defined above
  uint8_t language;            // Language Code - constants defined above
  ternary trafficProgram;      // Traffic Program flag - True if station gives
                               // Traffic Alerts
  ternary trafficAlert;        // Traffic Alert flag - True if station currently
                               // broadcasting Traffic Alert
  ternary
      music; // Music/speech flag - True if broadcasting music, false if speech
  ternary dynamicPTY;      // Dynamic PTY flag - True if dynamic (changing) PTY,
                           // false if static PTY
  ternary compressedAudio; // Compressed audio flag - True if compressed audio,
                           // false if not compressed
  ternary binauralAudio; // Binaural audio flag - True if binaural audio, false
                         // if not binaural audio
  ternary RDSStereo; // RDS stereo/mono flag - True if RDS info says station is
                     // stereo, false if mono
  char programService[9];  // Station's name or slogan - usually used like Radio
                           // Text
  uint8_t radioTextLen;    // Length of Radio Text message
  char radioText[65];      // Descriptive message from station
  char programTypeName[9]; // Program Type Name (PTYN)
  unsigned long MJD; // UTC Modified Julian Date - origin is November 17, 1858
  uint8_t hour;      // UTC Hour
  uint8_t minute;    // UTC Minute
  signed char
      offset; // Offset measured in half hours to convert UTC to local time.
              // If offset==NO_DATE_TIME then MJD, hour, minute are invalid.
} RDS;

typedef struct DateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t wday; // Day of the week, Sunday = 0
  uint8_t hour;
  uint8_t minute;
} DateTime;

typedef struct Time {
  uint8_t hour;
  uint8_t minute;
} Time;

extern RDS rds;

void SI4732_Init();
void SI4732_PowerUp();
void SI4732_PowerUpAlt();
void SI4732_PowerDown();
void SI4732_SetFreq(uint32_t freq);
uint8_t SI4732_GetRSSI();
uint8_t SI4732_GetSNR();
bool SI4732_GetRDS();
bool SI4732_GetLocalDateTime(DateTime *time);
bool SI4732_GetLocalTime(Time *time);

extern si47x_rds_status rdsResponse;

void RSQ_GET();

#endif /* end of include guard: SI473X_H */
