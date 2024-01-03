#ifndef RADIO_H
#define RADIO_H

#include "driver/bk4819.h"
#include "settings.h"
#include <stdbool.h>
#include <stdint.h>

extern VFO *gCurrentVFO;
extern VFO gVFO[2];
extern char gVFONames[2][10];

extern const char *upConverterFreqNames[3];
extern bool gIsListening;
extern bool gMonitorMode;
extern bool isBK1080;

extern const uint16_t StepFrequencyTable[14];

extern const char *modulationTypeOptions[6];
extern const char *vfoStateNames[];
extern const char *powerNames[];
extern const char *bwNames[3];
extern const char *deviationNames[];
extern const SquelchType sqTypeValues[4];
extern const char *sqTypeNames[4];

void RADIO_SetupRegisters();
uint32_t GetScreenF(uint32_t f);
uint32_t GetTuneF(uint32_t f);
void RADIO_ToggleRX(bool on);
void RADIO_ToggleModulation();
void RADIO_ToggleListeningBW();
void RADIO_UpdateStep(bool inc);
void RADIO_TuneTo(uint32_t f);
void RADIO_TuneToSave(uint32_t f);
void RADIO_SaveCurrentVFO();
void RADIO_LoadCurrentVFO();
void RADIO_SetSquelch(uint8_t sq);
void RADIO_SetGain(uint8_t gainIndex);
void RADIO_SetupByCurrentVFO();
void RADIO_SetupBandParams(Band *b);
void RADIO_EnableToneDetection();
void RADIO_NextCH(bool next);
void RADIO_NextVFO(bool next);
void RADIO_ToggleVfoMR();
void RADIO_UpdateSquelchLevel(bool next);
void RADIO_NextFreq(bool next);

#endif /* end of include guard: RADIO_H */
