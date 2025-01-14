#ifndef CHANNELS_H
#define CHANNELS_H

#define SCANLIST_MAX 1024

#include "../settings.h"
#include <stdint.h>

typedef struct {
  uint8_t value;
  uint8_t type : 4;
} Code;

typedef struct {
  Code rx;
  Code tx;
} CodeRXTX;

typedef struct {
  uint8_t s;
  uint8_t m;
  uint8_t e;
} __attribute__((packed)) PowerCalibration;

typedef struct {
  uint8_t value : 4;
  SquelchType type : 2;
} Squelch;

typedef enum {
  TYPE_EMPTY,
  TYPE_CH,
  TYPE_BAND,
  TYPE_VFO,
  TYPE_FOLDER,
  TYPE_MELODY,
  TYPE_SETTING,
  TYPE_FILE,

  TYPE_BAND_DETACHED = TYPE_EMPTY,
} CHType;

typedef enum {
  TYPE_FILTER_BAND = (1 << TYPE_BAND),
  TYPE_FILTER_BAND_SAVE = (1 << TYPE_BAND) | (1 << TYPE_EMPTY),
  TYPE_FILTER_CH = (1 << TYPE_CH),
  TYPE_FILTER_CH_SAVE = (1 << TYPE_CH) | (1 << TYPE_EMPTY),
} CHTypeFilter;

typedef enum {
  STEP_0_02kHz,
  STEP_0_05kHz,
  STEP_0_5kHz,
  STEP_1_0kHz,

  STEP_2_5kHz,
  STEP_5_0kHz,
  STEP_6_25kHz,
  STEP_8_33kHz,
  STEP_9_0kHz,
  STEP_10_0kHz,
  STEP_12_5kHz,
  STEP_25_0kHz,
  STEP_50_0kHz,
  STEP_100_0kHz,
  STEP_500_0kHz,
} Step;

typedef enum {
  OFFSET_NONE,
  OFFSET_PLUS,
  OFFSET_MINUS,
  OFFSET_FREQ,
} OffsetDirection;

typedef enum {
  TX_POW_ULOW,
  TX_POW_LOW,
  TX_POW_MID,
  TX_POW_HIGH,
} TXOutputPower;

typedef enum {
  RADIO_BK4819,
  RADIO_BK1080,
  RADIO_SI4732,
} Radio;

typedef struct {
  CHType type : 3;
  bool readonly : 1;
} CHMeta;

typedef struct {
  // Common fields
  CHMeta meta;
  union {
    uint16_t scanlists;
    int16_t channel;
  };
  char name[10];
  union {
    struct {
      uint32_t rxF : 27;
      int8_t ppm : 5;

      uint32_t txF : 27;

      OffsetDirection offsetDir : 2; // =0 -> tx=rxF
                                     // =1 -> tx=rxF+txF
                                     // =2 -> tx=rxF-txF
                                     // =4 -> tx=txF
      bool allowTx : 1;
      uint8_t reserved2 : 2;

      // Common radio settings
      Step step : 4;
      ModulationType modulation : 4;

      BK4819_FilterBandwidth_t bw : 4;
      Radio radio : 2;
      TXOutputPower power : 2;

      uint8_t scrambler : 4;
      Squelch squelch;

      union {
        // Only VFO/MR
        struct {
          CodeRXTX code; // 4B
          bool fixedBoundsMode : 1;
        };

        // Only BAND
        struct {
          uint8_t bank;
          PowerCalibration powCalib;
          // ^4B
          uint32_t lastUsedFreq : 27;
        } misc;
      };

      uint8_t gainIndex : 5; // Common rest
    } __attribute__((packed));
  };
} __attribute__((packed)) MR;
// getsize(MR); // 40
typedef MR Band;
typedef MR VFO;
typedef MR CH;

uint16_t CHANNELS_GetCountMax();

void CHANNELS_Load(int16_t num, CH *p);
void CHANNELS_Save(int16_t num, CH *p);
bool CHANNELS_LoadBuf();
void CHANNELS_Next(bool next);
void CHANNELS_Delete(int16_t i);
bool CHANNELS_Existing(int16_t i);
uint8_t CHANNELS_Scanlists(int16_t i);
void CHANNELS_LoadScanlist(CHTypeFilter type, uint16_t n);
void CHANNELS_LoadBlacklistToLoot();

uint16_t CHANNELS_GetStepSize(CH *p);
uint32_t CHANNELS_GetSteps(Band *p);
uint32_t CHANNELS_GetF(Band *p, uint32_t channel);
uint32_t CHANNELS_GetChannel(Band *p, uint32_t f);
CHMeta CHANNELS_GetMeta(int16_t num);

bool CHANNELS_IsScanlistable(CHType type);
bool CHANNELS_IsFreqable(CHType type);
uint16_t CHANNELS_ScanlistByKey(uint16_t sl, KEY_Code_t key, bool longPress);

extern int16_t gScanlistSize;
extern uint16_t gScanlist[SCANLIST_MAX];
extern const char *TX_POWER_NAMES[4];
extern const char *TX_OFFSET_NAMES[3];
extern const char *TX_CODE_TYPES[4];

#endif /* end of include guard: CHANNELS_H */
