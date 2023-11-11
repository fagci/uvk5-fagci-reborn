#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>

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

extern const uint16_t StepFrequencyTable[12];

extern const char *modulationTypeOptions[5];
extern const char *vfoStateNames[];
extern const char *powerNames[];
extern const char *bwNames[3];
extern const char *deviationNames[];

void RADIO_SetupRegisters();
uint32_t GetScreenF(uint32_t f);
uint32_t GetTuneF(uint32_t f);

#endif /* end of include guard: RADIO_H */
