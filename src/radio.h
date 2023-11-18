#ifndef RADIO_H
#define RADIO_H

#include <stdbool.h>
#include <stdint.h>

#define CHANNELS_COUNT 400
#define VFO_SIZE 16

typedef struct {
  uint32_t fRX : 32; // 4
  uint32_t fTX : 32; // 4
  char name[16]; // 2
  uint8_t memoryBanks : 8; // 1
  uint8_t step : 8; // 1
  uint8_t modulation : 4;
  uint8_t bw : 2;
  uint8_t power : 2; // 1
  uint8_t codeRx : 8; // 1
  uint8_t codeTx : 8; // 1
  uint8_t codeTypeRx : 4;
  uint8_t codeTypeTx : 4; // 1
} VFO;

extern VFO gCurrentVfo;

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

extern VFO gCurrentVfo;
extern UpconverterTypes gUpconverterType;
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
void RADIO_TuneTo(uint32_t f, bool precise);

#endif /* end of include guard: RADIO_H */
