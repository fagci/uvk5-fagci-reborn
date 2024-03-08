#ifndef CHANNELS_H
#define CHANNELS_H

#include "../driver/bk4819.h"
#include "../globals.h"
#include "../helper/appsregistry.h"
#include <stdint.h>

typedef enum {
  CH_CHANNEL,
  CH_VFO,
  CH_PRESET,
} ChannelType;

typedef enum {
  SCAN_TO_0,
  SCAN_TO_500ms,
  SCAN_TO_1s,
  SCAN_TO_2s,
  SCAN_TO_5s,
  SCAN_TO_10s,
  SCAN_TO_30s,
  SCAN_TO_1min,
  SCAN_TO_2min,
  SCAN_TO_5min,
  SCAN_TO_NONE,
} ScanTimeout;

typedef struct {
  uint8_t timeout : 8;
  ScanTimeout openedTimeout : 4;
  ScanTimeout closedTimeout : 4;
} __attribute__((packed)) ScanSettings;

typedef struct {
  uint8_t level : 6;
  uint8_t openTime : 2;
  SquelchType type;
  uint8_t closeTime : 3;
} __attribute__((packed)) SquelchSettings;
// getsize(SquelchSettings)

typedef struct {
  AppType_t app;
  int16_t channel;
  ScanSettings scan;
} VFO_Params;

typedef struct {
  ChannelType type;
  union {
    char name[10];
    VFO_Params vfo;
  };
  uint32_t f : 27;
  uint32_t offset : 27;
  OffsetDirection offsetDir;
  ModulationType modulation : 4;
  BK4819_FilterBandwidth_t bw : 2;
  TXOutputPower power : 2;
  uint8_t codeRX;
  uint8_t codeTX;
  uint8_t codeTypeRX : 4;
  uint8_t codeTypeTX : 4;
  uint8_t groups;
  SquelchSettings sq;
  uint8_t gainIndex : 5;
  Step step : 4;
} __attribute__((packed)) CH; // 33 B
// getsize(CH)

int16_t CHANNELS_GetCountMax();
void CHANNELS_Load(int16_t num, CH *p);
void CHANNELS_Save(int16_t num, CH *p);
CH *CHANNELS_Get(int16_t i);
bool CHANNELS_LoadBuf();
int16_t CHANNELS_Next(int16_t base, bool next);
void CHANNELS_Delete(int16_t i);
bool CHANNELS_Existing(int16_t i);
uint8_t CHANNELS_Scanlists(int16_t i);
void CHANNELS_LoadScanlist(uint8_t n);

extern int16_t gScanlistSize;
extern int32_t gScanlist[350];

#endif /* end of include guard: CHANNELS_H */
