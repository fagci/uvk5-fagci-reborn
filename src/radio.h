#ifndef RADIO_H
#define RADIO_H

#include "driver/bk4819.h"
#include "helper/lootlist.h"
#include "settings.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  TX_UNKNOWN,
  TX_ON,
  TX_VOL_HIGH,
  TX_BAT_LOW,
  TX_DISABLED,
} TXState;

extern VFO *gCurrentVFO;
extern VFO gVFO[2];
extern Preset *gVFOPresets[2];

extern Loot *gCurrentLoot;
extern Loot gLoot[2];

extern char gVFONames[2][10];

extern bool gIsListening;
extern bool gMonitorMode;
extern bool isBK1080;
extern TXState gTxState;

extern const uint16_t StepFrequencyTable[14];
extern const char *modulationTypeOptions[6];
extern const SquelchType sqTypeValues[4];

extern const char *upConverterFreqNames[3];
extern const char *vfoStateNames[];
extern const char *powerNames[];
extern const char *bwNames[3];
extern const char *deviationNames[];
extern const char *sqTypeNames[4];
extern const char *TX_STATE_NAMES[5];

void RADIO_SetupRegisters();

void RADIO_SaveCurrentVFO();
void RADIO_LoadCurrentVFO();

void RADIO_ToggleRX(bool on);
void RADIO_ToggleTX(bool on);

void RADIO_TuneTo(uint32_t f);
void RADIO_TuneToPure(uint32_t f);
void RADIO_TuneToSave(uint32_t f);
void RADIO_SetupBandParams(Band *b);

void RADIO_EnableToneDetection();

void RADIO_VfoLoadCH(uint8_t i);
void RADIO_SetupByCurrentVFO();
void RADIO_NextVFO(bool next);
void RADIO_NextCH(bool next);
void RADIO_NextFreq(bool next);
void RADIO_NextPresetFreq(bool next);
void RADIO_ToggleVfoMR();

void RADIO_SetSquelchPure(uint32_t f, uint8_t sql);
void RADIO_SetSquelch(uint8_t sq);
void RADIO_SetGain(uint8_t gainIndex);
void RADIO_ToggleModulation();
void RADIO_ToggleListeningBW();
void RADIO_UpdateStep(bool inc);
void RADIO_UpdateSquelchLevel(bool next);

uint32_t GetScreenF(uint32_t f);
uint32_t GetTuneF(uint32_t f);

bool RADIO_IsBK1080Range(uint32_t f);

void RADIO_UpdateMeasurements();
bool RADIO_UpdateMeasurementsEx(Loot *dest);

#endif /* end of include guard: RADIO_H */
