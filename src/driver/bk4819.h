/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#ifndef DRIVER_BK4819_h
#define DRIVER_BK4819_h

#include "../driver/bk4819-regs.h"
#include <stdint.h>

#define VHF_UHF_BOUND1 24000000
#define VHF_UHF_BOUND2 28000000

#define F_MIN 0
#define F_MAX 130000000

typedef enum {
  FILTER_VHF,
  FILTER_UHF,
  FILTER_OFF,
} Filter;

enum BK4819_AF_Type_t {
  BK4819_AF_MUTE,
  BK4819_AF_FM,
  BK4819_AF_ALAM, // tone
  BK4819_AF_BEEP, // for tx
  BK4819_AF_RAW, // (ssb without if filter = raw in sdr sharp)
  BK4819_AF_USB, // (or ssb = lsb and usb at the same time)
  BK4819_AF_CTCO, // ctcss/dcs (fm with narrow filters for ctcss/dcs)
  BK4819_AF_AM,
  BK4819_AF_FSKO, // fsk out test with special fsk filters (need reg58 fsk on to give sound on speaker )
  BK4819_AF_BYPASS, // (fm without filter = discriminator output)
};

typedef enum {
  MOD_FM,
  MOD_AM,
  MOD_USB,
  MOD_BYP,
  MOD_RAW,
  MOD_WFM,
} ModulationType;

typedef enum {
  SQUELCH_RSSI_NOISE_GLITCH,
  SQUELCH_RSSI_GLITCH,
  SQUELCH_RSSI_NOISE,
  SQUELCH_RSSI,
} SquelchType;

typedef enum BK4819_AF_Type_t BK4819_AF_Type_t;

enum BK4819_FilterBandwidth_t {
  BK4819_FILTER_BW_WIDE,
  BK4819_FILTER_BW_NARROW,
  BK4819_FILTER_BW_NARROWER,
};

typedef enum BK4819_FilterBandwidth_t BK4819_FilterBandwidth_t;

enum BK4819_CssScanResult_t {
  BK4819_CSS_RESULT_NOT_FOUND,
  BK4819_CSS_RESULT_CTCSS,
  BK4819_CSS_RESULT_CDCSS,
};

typedef enum {
  F_SC_T_0_2s,
  F_SC_T_0_4s,
  F_SC_T_0_8s,
  F_SC_T_1_6s,
} FreqScanTime;

typedef struct {
  uint16_t regValue;
  int8_t gainDb;
} Gain;

typedef enum BK4819_CssScanResult_t BK4819_CssScanResult_t;
extern const uint16_t BWRegValues[3];
extern const Gain gainTable[19];

extern bool gRxIdleMode;
extern const uint8_t SQ[2][6][11];

void BK4819_Init();
uint16_t BK4819_ReadRegister(BK4819_REGISTER_t Register);
void BK4819_WriteRegister(BK4819_REGISTER_t Register, uint16_t Data);
void BK4819_WriteU8(uint8_t Data);
void BK4819_WriteU16(uint16_t Data);

void BK4819_SetAGC(bool useDefault);

void BK4819_ToggleGpioOut(BK4819_GPIO_PIN_t Pin, bool bSet);

void BK4819_SetCDCSSCodeWord(uint32_t CodeWord);
void BK4819_SetCTCSSFrequency(uint32_t BaudRate);
void BK4819_SetTailDetection(const uint32_t freq_10Hz);
void BK4819_EnableVox(uint16_t Vox1Threshold, uint16_t Vox0Threshold);
void BK4819_SetFilterBandwidth(BK4819_FilterBandwidth_t Bandwidth);
void BK4819_SetupPowerAmplifier(uint16_t Bias, uint32_t Frequency);
void BK4819_SetFrequency(uint32_t Frequency);
uint32_t BK4819_GetFrequency();
void BK4819_SetupSquelch(uint8_t SquelchOpenRSSIThresh,
                         uint8_t SquelchCloseRSSIThresh,
                         uint8_t SquelchOpenNoiseThresh,
                         uint8_t SquelchCloseNoiseThresh,
                         uint8_t SquelchCloseGlitchThresh,
                         uint8_t SquelchOpenGlitchThresh, uint8_t OpenDelay,
                         uint8_t CloseDelay);
void BK4819_Squelch(uint8_t sql, uint32_t f, uint8_t OpenDelay,
                    uint8_t CloseDelay);
void BK4819_SquelchType(SquelchType t);

void BK4819_SetAF(BK4819_AF_Type_t AF);
void BK4819_RX_TurnOn();
void BK4819_DisableFilter();
void BK4819_SelectFilter(uint32_t Frequency);
void BK4819_DisableScramble();
void BK4819_EnableScramble(uint8_t Type);
void BK4819_DisableVox();
void BK4819_DisableDTMF();
void BK4819_EnableDTMF();
void BK4819_PlayTone(uint16_t Frequency, bool bTuningGainSwitch);
void BK4819_EnterTxMute();
void BK4819_ExitTxMute();
void BK4819_Sleep();
void BK4819_TurnsOffTones_TurnsOnRX();
void BK4819_SetupAircopy();
void BK4819_ResetFSK();
void BK4819_FskClearFifo();
void BK4819_FskEnableRx();
void BK4819_FskEnableTx();
void BK4819_Idle();
void BK4819_ExitBypass();
void BK4819_PrepareTransmit();
void BK4819_TxOn_Beep();
void BK4819_ExitSubAu();

void BK4819_EnableRX();

void BK4819_EnterDTMF_TX(bool bLocalLoopback);
void BK4819_ExitDTMF_TX(bool bKeep);
void BK4819_EnableTXLink();

void BK4819_PlayDTMF(char Code);
void BK4819_PlayDTMFString(const char *pString, bool bDelayFirst,
                           uint16_t FirstCodePersistTime,
                           uint16_t HashCodePersistTime,
                           uint16_t CodePersistTime, uint16_t CodeInternalTime);

void BK4819_TransmitTone(bool bLocalLoopback, uint32_t Frequency);

void BK4819_GenTail(uint8_t Tail);
void BK4819_EnableCDCSS();
void BK4819_EnableCTCSS();

uint16_t BK4819_GetRSSI();
uint8_t BK4819_GetNoise();
uint8_t BK4819_GetGlitch();
uint8_t BK4819_GetSNR();

bool BK4819_GetFrequencyScanResult(uint32_t *pFrequency);
BK4819_CssScanResult_t BK4819_GetCxCSSScanResult(uint32_t *pCdcssFreq,
                                                 uint16_t *pCtcssFreq);
void BK4819_DisableFrequencyScan();
void BK4819_EnableFrequencyScan();
void BK4819_EnableFrequencyScanEx(FreqScanTime t);
void BK4819_SetScanFrequency(uint32_t Frequency);

void BK4819_StopScan();

uint8_t BK4819_GetDTMF_5TONE_Code();

uint8_t BK4819_GetCDCSSCodeType();
uint8_t BK4819_GetCTCType();

void BK4819_SendFSKData(uint16_t *pData);
void BK4819_PrepareFSKReceive();

void BK4819_PlayRoger();
void BK4819_PlayRogerMDC();

void BK4819_Enable_AfDac_DiscMode_TxDsp();

void BK4819_GetVoxAmp(uint16_t *pResult);
void BK4819_PlayDTMFEx(bool bLocalLoopback, char Code);

void BK4819_ToggleAFBit(bool on);
void BK4819_ToggleAFDAC(bool on);
uint16_t BK4819_GetRegValue(RegisterSpec s);
void BK4819_SetRegValue(RegisterSpec s, uint16_t v);
void BK4819_TuneTo(uint32_t f, bool precise);
void BK4819_SetToneFrequency(uint16_t f);
void BK4819_SetModulation(ModulationType type);
bool BK4819_IsSquelchOpen();
void BK4819_ResetRSSI();
void BK4819_SetGain(uint8_t gainIndex);
void BK4819_HandleInterrupts(void (*handler)(uint16_t intStatus));

#endif
