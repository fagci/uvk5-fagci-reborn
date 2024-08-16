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
  TX_DISABLED_UPCONVERTER,
  TX_POW_OVERDRIVE,
} TXState;

extern VFO *radio;
extern VFO gVFO[2];
extern Preset *gVFOPresets[2];

extern Loot *gCurrentLoot;
extern Loot gLoot[2];

extern char gVFONames[2][10];

extern bool gIsListening;
extern bool gMonitorMode;
extern TXState gTxState;

extern const uint16_t StepFrequencyTable[14];
extern const char *modulationTypeOptions[8];
extern const SquelchType sqTypeValues[4];

extern const char *upConverterFreqNames[3];
extern const char *vfoStateNames[];
extern const char *powerNames[];
extern const char *radioNames[4];
extern const char *shortRadioNames[4];
extern const char *bwNames[3];
extern const char *deviationNames[];
extern const char *sqTypeNames[4];
extern const char *TX_STATE_NAMES[7];

Radio RADIO_GetRadio();
ModulationType RADIO_GetModulation();
void RADIO_SetupRegisters();

void RADIO_SaveCurrentVFO();
void RADIO_LoadCurrentVFO();

void RADIO_ToggleRX(bool on);
void RADIO_ToggleTX(bool on);

void RADIO_TuneTo(uint32_t f);
bool RADIO_TuneToCH(int32_t num);
void RADIO_TuneToPure(uint32_t f, bool precise);
void RADIO_TuneToSave(uint32_t f);
void RADIO_SelectPreset(int8_t num);
void RADIO_SelectPresetSave(int8_t num);
void RADIO_SetupBandParams();

void RADIO_VfoLoadCH(uint8_t i);
void RADIO_SetupByCurrentVFO();
void RADIO_NextVFO(void);
void RADIO_NextCH(bool next);
void RADIO_NextFreqNoClicks(bool next);
void RADIO_NextPresetFreq(bool next);
void RADIO_NextPresetFreqEx(bool next, bool precise);
void RADIO_ToggleVfoMR();

void RADIO_SetSquelchPure(uint32_t f, uint8_t sql);
void RADIO_SetSquelch(uint8_t sq);
void RADIO_SetGain(uint8_t gainIndex);
void RADIO_ToggleModulation();
void RADIO_ToggleListeningBW();
void RADIO_ToggleTxPower(void);
void RADIO_UpdateStep(bool inc);
void RADIO_UpdateSquelchLevel(bool next);

uint32_t GetScreenF(uint32_t f);
uint32_t GetTuneF(uint32_t f);
uint16_t RADIO_GetRSSI(void);
uint32_t RADIO_GetTXF(void);
uint32_t RADIO_GetTXFEx(VFO *vfo, Preset *p);
void RADIO_ToggleBK1080(bool on);

bool RADIO_IsBK1080Range(uint32_t f);

Loot *RADIO_UpdateMeasurements();
bool RADIO_UpdateMeasurementsEx(Loot *dest);

#endif /* end of include guard: RADIO_H */
