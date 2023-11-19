#ifndef RADIO_H
#define RADIO_H

#include <stdbool.h>
#include <stdint.h>

#define BANDS_COUNT 32
#define CHANNELS_COUNT 300

#define BAND_SIZE 28
#define VFO_SIZE 24

#define BANDS_OFFSET 0
#define CHANNELS_OFFSET (BANDS_OFFSET + BAND_SIZE * BANDS_COUNT)
#define CURRENT_VFO_OFFSET (CHANNELS_OFFSET + VFO_SIZE * CHANNELS_COUNT)

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

typedef struct {           // 24 bytes
  uint32_t fRX : 32;       // 4
  uint32_t fTX : 32;       // 4
  char name[10];           // 10
  uint8_t memoryBanks : 8; // 1
  uint8_t step : 4;
  uint8_t modulation : 4; // 1
  uint8_t bw : 2;
  uint8_t power : 2;
  uint8_t codeTypeRx : 4; // 1
  uint8_t codeTypeTx : 4;
  uint8_t codeRx : 8; // 1
  uint8_t codeTx : 8; // 1
} VFO;

typedef struct {           // 28 bytes
  uint32_t fStart : 32;    // 4
  uint32_t fEnd : 32;      // 4
  uint32_t offset : 32;    // 4
  char name[10];           // 10
  uint8_t memoryBanks : 8; // 1
  uint8_t step : 4;
  uint8_t modulation : 4; // 1
  uint8_t bw : 2;
  uint8_t power : 2;
  uint8_t codeTypeRx : 4; // 1
  uint8_t codeTypeTx : 4;
  uint8_t codeRx : 8; // 1
  uint8_t codeTx : 8; // 1
  uint8_t squelch : 4;
  uint8_t squelchType : 2; // 1
} Band;

extern VFO gCurrentVfo;
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
void RADIO_TuneTo(uint32_t f, bool precise);
void RADIO_SaveCurrentVFO();
void RADIO_LoadCurrentVFO();

#endif /* end of include guard: RADIO_H */
