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

#ifndef DRIVER_BK1080_H
#define DRIVER_BK1080_H

#include "bk1080-regs.h"
#include <stdint.h>

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

#endif
