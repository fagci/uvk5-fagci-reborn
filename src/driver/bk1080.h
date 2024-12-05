#ifndef DRIVER_BK1080_H
#define DRIVER_BK1080_H

#include "bk1080-regs.h"
#include <stdbool.h>
#include <stdint.h>

#define BK1080_F_MIN 6400000
#define BK1080_F_MAX 10800000

typedef enum {
  BK1080_BAND_87_5_108,
  BK1080_BAND_76_108,
  BK1080_BAND_76_90,
  BK1080_BAND_64_76,
} BK1080_Band;

typedef enum {
  BK1080_CHSP_200,
  BK1080_CHSP_100,
  BK1080_CHSP_50,
} BK1080_ChannelSpacing;

void BK1080_Init(uint32_t Frequency, bool bEnable);
uint16_t BK1080_ReadRegister(BK1080_Register_t Register);
void BK1080_WriteRegister(BK1080_Register_t Register, uint16_t Value);
void BK1080_Mute(bool Mute);
void BK1080_SetFrequency(uint32_t Frequency);
uint16_t BK1080_GetFrequencyDeviation();
uint16_t BK1080_GetRSSI();
uint8_t BK1080_GetSNR();

#endif
