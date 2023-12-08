#ifndef RADIO_H
#define RADIO_H

#include "driver/bk4819.h"
#include <stdbool.h>
#include <stdint.h>

#define BANDS_COUNT 28
#define CHANNELS_COUNT 300

#define PRESET_SIZE sizeof(Preset)
#define VFO_SIZE sizeof(VFO)

#define BANDS_OFFSET 0
#define CHANNELS_OFFSET (BANDS_OFFSET + PRESET_SIZE * BANDS_COUNT)

typedef enum {
  STEP_0_01kHz,
  STEP_0_1kHz,
  STEP_0_5kHz,
  STEP_1_0kHz,

  STEP_2_5kHz,
  STEP_5_0kHz,
  STEP_6_25kHz,
  STEP_8_33kHz,
  STEP_10_0kHz,
  STEP_12_5kHz,
  STEP_25_0kHz,
  STEP_100_0kHz,
} Step;

typedef enum {
  UPCONVERTER_OFF,
  UPCONVERTER_50M,
  UPCONVERTER_125M,
} UpconverterTypes;

typedef struct {                 // 24 bytes
  uint32_t fRX;                  // 4
  uint32_t fTX;                  // 4
  char name[10];                 // 10
  uint8_t memoryBanks : 8;       // 1
  ModulationType modulation : 4; // 1
  BK4819_FilterBandwidth_t bw : 2;
  uint8_t power : 2;
  uint8_t codeTypeRx : 4;
  uint8_t codeTypeTx : 4; // 1
  uint8_t codeRx : 8;     // 1
  uint8_t codeTx : 8;     // 1
  uint8_t reserved : 8;   // 1
} __attribute__((packed)) VFO;

typedef struct { // 8 bytes
  uint32_t start;
  uint32_t end;
} __attribute__((packed)) FRange;

typedef struct { // 21 bytes
  FRange bounds;
  char name[10];
  Step step : 4;
  ModulationType modulation : 4;
  BK4819_FilterBandwidth_t bw : 2;
  SquelchType squelchType : 2;
  uint8_t squelch : 4;
  uint8_t gainIndex : 7;
  bool reserved1 : 1;
} __attribute__((packed)) Band;

typedef struct { // 29 bytes
  Band band;
  uint32_t offset;         // 4
  uint8_t memoryBanks : 8; // 1
  uint8_t codeTypeRx : 4;
  uint8_t codeTypeTx : 4; // 1
  uint8_t codeRx : 8;     // 1
  uint8_t codeTx : 8;     // 1
  uint8_t power : 2;
  uint8_t a : 6;
  uint8_t b : 8;
  uint8_t c : 8;
} __attribute__((packed)) Preset;

extern VFO gCurrentVfo;
extern Preset gCurrentPreset;
extern const char *upConverterFreqNames[3];
extern bool gIsListening;

extern const uint16_t StepFrequencyTable[12];

extern const char *modulationTypeOptions[5];
extern const char *vfoStateNames[];
extern const char *powerNames[];
extern const char *bwNames[3];
extern const char *deviationNames[];

void RADIO_SetupRegisters();
uint32_t GetScreenF(uint32_t f);
uint32_t GetTuneF(uint32_t f);
void RADIO_ToggleRX(bool on);
void RADIO_ToggleModulation();
void RADIO_ToggleListeningBW();
void RADIO_UpdateStep(bool inc);
void RADIO_TuneTo(uint32_t f);
void RADIO_SaveCurrentVFO();
void RADIO_LoadCurrentVFO();
void RADIO_SetSquelch(uint8_t sq);
void RADIO_SetGain(uint8_t gainIndex);
void RADIO_SetupByCurrentVFO();
void RADIO_SetupBandParams(Band *b);
void RADIO_LoadChannel(uint16_t num, VFO *p);
void RADIO_SaveChannel(uint16_t num, VFO *p);
void RADIO_SavePreset(uint8_t num, Preset *p);
void RADIO_LoadPreset(uint8_t num, Preset *p);

#endif /* end of include guard: RADIO_H */
