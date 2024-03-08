#ifndef RADIO_H
#define RADIO_H

#include "frequency.h"
#include "helper/channels.h"
#include "helper/lootlist.h"
#include <stdint.h>

extern CH *radio;
extern Loot *gCurrentLoot;
extern FRange *gCurrentBounds;

extern bool gIsListening;
extern bool gMonitorMode;
extern bool gIsBK1080;
extern TXState gTxState;

void RADIO_SetupRegisters();

void RADIO_SaveCurrentCH();
void RADIO_LoadCurrentCH();

void RADIO_ToggleRX(bool on);
void RADIO_ToggleTX(bool on);

void RADIO_SetupParams();
void RADIO_TuneToPure(uint32_t f, bool precise);

void RADIO_EnableToneDetection();

void RADIO_VfoLoadCH(uint8_t i);
void RADIO_SetupByCurrentCH();
void RADIO_NextCH(bool next);
void RADIO_NextVFO();
void RADIO_NextFreq(bool next);
void RADIO_NextBandFreq(bool next);
void RADIO_NextBandFreqEx(bool next, bool precise);
void RADIO_ToggleVfoMR();

void RADIO_SetSquelch(uint8_t sq);
void RADIO_SetGain(uint8_t gainIndex);
void RADIO_ToggleModulation();
void RADIO_ToggleListeningBW();
void RADIO_ToggleTxPower();
void RADIO_UpdateStep(bool inc);
void RADIO_UpdateSquelchLevel(bool next);

uint16_t RADIO_GetRSSI();
uint32_t RADIO_GetTXFEx(CH *vfo);
void RADIO_ToggleBK1080(bool on);

bool RADIO_IsBK1080Range(uint32_t f);

Loot *RADIO_UpdateMeasurements();
bool RADIO_UpdateMeasurementsEx(Loot *dest);

#endif /* end of include guard: RADIO_H */
