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

#include "bk4819.h"
#include "../driver/gpio.h"
#include "../driver/system.h"
#include "../driver/systick.h"
#include "../driver/uart.h"
#include "../inc/dp32g030/gpio.h"
#include "../inc/dp32g030/portcon.h"
#include "../misc.h"
#include "../settings.h"
#include "bk4819-regs.h"

#define BK4819_F_MIN 1600000
#define BK4819_F_MAX 134000000

static const uint16_t FSK_RogerTable[7] = {
    0xF1A2, 0x7446, 0x61A4, 0x6544, 0x4E8A, 0xE044, 0xEA84,
};

static uint16_t gBK4819_GpioOutState;
static Filter selectedFilter = FILTER_OFF;
static uint8_t modTypeCurrent = 255;

bool gRxIdleMode;

const uint8_t DTMF_COEFFS[] = {111, 107, 103, 98, 80,  71,  58,  44,
                               65,  55,  37,  23, 228, 203, 181, 159};

/* const uint8_t SQ[2][6][11] = {
    {
        {0, 10, 44, 52, 58, 66, 72, 80, 88, 94, 102},
        {0, 5, 38, 46, 54, 62, 68, 76, 84, 92, 100},
        {255, 90, 53, 48, 44, 40, 36, 32, 28, 24, 20},
        {255, 100, 56, 52, 47, 43, 39, 35, 31, 27, 23},
        {255, 90, 32, 24, 20, 17, 14, 11, 8, 3, 2},
        {255, 100, 30, 21, 17, 14, 11, 8, 5, 5, 4},
    },
    {
        {0, 36, 77, 82, 88, 94, 100, 106, 112, 118, 123},
        {0, 40, 70, 76, 82, 88, 94, 102, 108, 114, 120},
        {255, 65, 58, 52, 46, 41, 37, 33, 28, 24, 22},
        {255, 70, 65, 57, 51, 45, 41, 37, 32, 28, 25},
        {255, 90, 32, 23, 18, 15, 10, 9, 8, 7, 4},
        {255, 100, 60, 45, 30, 20, 15, 13, 12, 11, 8},
    },
}; */

const uint8_t SQ[2][6][11] = {
    {
        {0, 10, 62, 66, 74, 75, 92, 95, 98, 170, 252},
        {0, 5, 60, 64, 72, 70, 89, 92, 95, 166, 250},
        {255, 240, 56, 54, 48, 45, 32, 29, 20, 25, 20},
        {255, 250, 61, 58, 52, 48, 35, 32, 23, 30, 30},
        {255, 240, 135, 135, 116, 17, 3, 3, 2, 50, 50},
        {255, 250, 150, 140, 120, 20, 5, 5, 4, 45, 45},
    },
    {
        {0, 50, 78, 88, 94, 110, 114, 117, 119, 200, 252},
        {0, 40, 76, 86, 92, 106, 110, 113, 115, 195, 250},
        {255, 65, 49, 44, 42, 40, 33, 30, 22, 23, 22},
        {255, 70, 59, 54, 46, 45, 37, 34, 25, 27, 25},
        {255, 90, 135, 135, 116, 10, 8, 7, 6, 32, 32},
        {255, 100, 150, 140, 120, 15, 12, 11, 10, 30, 30},
    },
};

const uint16_t BWRegValues[3] = {0x3028, 0x4048, 0x0018};

const Gain gainTable[19] = {
    {0x000, -43}, //
    {0x100, -40}, //
    {0x020, -38}, //
    {0x200, -35}, //
    {0x040, -33}, //
    {0x220, -30}, //
    {0x060, -28}, //
    {0x240, -25}, //
    {0x0A0, -23}, //
    {0x260, -20}, //
    {0x1C0, -18}, //
    {0x2A0, -15}, //
    {0x2C0, -13}, //
    {0x2E0, -11}, //
    {0x360, -9},  //
    {0x380, -6},  //
    {0x3A0, -4},  //
    {0x3C0, -2},  //
    {0x3E0, 0},   //
};

void BK4819_Init() {
  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);
  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);

  BK4819_WriteRegister(BK4819_REG_00, 0x8000);
  BK4819_WriteRegister(BK4819_REG_00, 0x0000);
  BK4819_WriteRegister(BK4819_REG_37, 0x1D0F);
  BK4819_WriteRegister(BK4819_REG_36, 0x0022);
  BK4819_SetAGC(true);
  BK4819_WriteRegister(BK4819_REG_19, 0x1041);
  BK4819_WriteRegister(BK4819_REG_7D, 0xE94F);
  BK4819_WriteRegister(BK4819_REG_48, 0xB3A8);

  for (uint8_t i = 0; i < ARRAY_SIZE(DTMF_COEFFS); ++i) {
    BK4819_WriteRegister(0x09, (i << 12) | DTMF_COEFFS[i]);
  }

  BK4819_WriteRegister(BK4819_REG_1F, 0x5454);
  BK4819_WriteRegister(BK4819_REG_3E, 0xA037);
  gBK4819_GpioOutState = 0x9000;
  BK4819_WriteRegister(BK4819_REG_33, 0x9000);
  BK4819_WriteRegister(BK4819_REG_3F, 0);
}

void BK4819_WriteU8(uint8_t Data) {
  uint8_t i;

  GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
  for (i = 0; i < 8; i++) {
    if ((Data & 0x80U) == 0) {
      GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);
    } else {
      GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);
    }
    SYSTICK_DelayUs(1);
    GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
    SYSTICK_DelayUs(1);
    Data <<= 1;
    GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
    SYSTICK_DelayUs(1);
  }
}

static uint16_t BK4819_ReadU16() {
  uint8_t i;
  uint16_t Value;

  PORTCON_PORTC_IE = (PORTCON_PORTC_IE & ~PORTCON_PORTC_IE_C2_MASK) |
                     PORTCON_PORTC_IE_C2_BITS_ENABLE;
  GPIOC->DIR = (GPIOC->DIR & ~GPIO_DIR_2_MASK) | GPIO_DIR_2_BITS_INPUT;
  SYSTICK_DelayUs(1);

  Value = 0;
  for (i = 0; i < 16; i++) {
    Value <<= 1;
    Value |= GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);
    GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
    SYSTICK_DelayUs(1);
    GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
    SYSTICK_DelayUs(1);
  }
  PORTCON_PORTC_IE = (PORTCON_PORTC_IE & ~PORTCON_PORTC_IE_C2_MASK) |
                     PORTCON_PORTC_IE_C2_BITS_DISABLE;
  GPIOC->DIR = (GPIOC->DIR & ~GPIO_DIR_2_MASK) | GPIO_DIR_2_BITS_OUTPUT;

  return Value;
}

void BK4819_WriteU16(uint16_t Data) {
  GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
  for (uint8_t i = 0; i < 16; i++) {
    if ((Data & 0x8000U) == 0U) {
      GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);
    } else {
      GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);
    }
    SYSTICK_DelayUs(1);
    GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
    Data <<= 1;
    SYSTICK_DelayUs(1);
    GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
    SYSTICK_DelayUs(1);
  }
}

uint16_t BK4819_ReadRegister(BK4819_REGISTER_t Register) {
  uint16_t Value;

  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);
  GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
  SYSTICK_DelayUs(1);
  GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);

  BK4819_WriteU8(Register | 0x80);

  Value = BK4819_ReadU16();

  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);
  SYSTICK_DelayUs(1);
  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);

  return Value;
}

void BK4819_WriteRegister(BK4819_REGISTER_t Register, uint16_t Data) {
  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);
  GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
  SYSTICK_DelayUs(1);
  GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);
  BK4819_WriteU8(Register);
  SYSTICK_DelayUs(1);
  BK4819_WriteU16(Data);
  SYSTICK_DelayUs(1);
  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);
  SYSTICK_DelayUs(1);
  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);
}

void BK4819_SetAGC(bool useDefault) {
  // QS
  BK4819_WriteRegister(BK4819_REG_13, 0x03BE);
  BK4819_WriteRegister(BK4819_REG_12, 0x037B);
  BK4819_WriteRegister(BK4819_REG_11, 0x027B);
  BK4819_WriteRegister(BK4819_REG_10, 0x007A);

  // BK
  /* BK4819_WriteRegister(BK4819_REG_13, 0x03DE);
  BK4819_WriteRegister(BK4819_REG_12, 0x037B);
  BK4819_WriteRegister(BK4819_REG_11, 0x025A);
  BK4819_WriteRegister(BK4819_REG_10, 0x0038); */

  // 1o11
  /* BK4819_WriteRegister(BK4819_REG_12, 0x0393);
  BK4819_WriteRegister(BK4819_REG_11, 0x01B5);
  BK4819_WriteRegister(BK4819_REG_10, 0x0145);
  BK4819_WriteRegister(BK4819_REG_14, 0x0019); */

  uint8_t Lo = 0;    // 0-1 - auto, 2 - low, 3 high
  uint8_t low = 48;  // 1dB / LSB 56
  uint8_t high = 80; // 1dB / LSB 84

  if (useDefault) {
    BK4819_WriteRegister(BK4819_REG_14, 0x0019);
  } else {
    BK4819_WriteRegister(BK4819_REG_14, 0x0000);
    // slow 25 45
    // fast 15 50
    low = 15;
    high = 50;
  }
  BK4819_WriteRegister(BK4819_REG_49, (Lo << 14) | (high << 7) | (low << 0));
  BK4819_WriteRegister(BK4819_REG_7B, 0x8420);
}

void BK4819_ToggleGpioOut(BK4819_GPIO_PIN_t Pin, bool bSet) {
  if (bSet) {
    gBK4819_GpioOutState |= (0x40U >> Pin);
  } else {
    gBK4819_GpioOutState &= ~(0x40U >> Pin);
  }

  BK4819_WriteRegister(BK4819_REG_33, gBK4819_GpioOutState);
}

void BK4819_SetCDCSSCodeWord(uint32_t CodeWord) {
  // Enable CDCSS
  // Transmit positive CDCSS code
  // CDCSS Mode
  // CDCSS 23bit
  // Enable Auto CDCSS Bw Mode
  // Enable Auto CTCSS Bw Mode
  // CTCSS/CDCSS Tx Gain1 Tuning = 51
  BK4819_WriteRegister(
      BK4819_REG_51,
      0 | BK4819_REG_51_ENABLE_CxCSS | BK4819_REG_51_GPIO6_PIN2_NORMAL |
          BK4819_REG_51_TX_CDCSS_POSITIVE | BK4819_REG_51_MODE_CDCSS |
          BK4819_REG_51_CDCSS_23_BIT | BK4819_REG_51_1050HZ_NO_DETECTION |
          BK4819_REG_51_AUTO_CDCSS_BW_ENABLE |
          BK4819_REG_51_AUTO_CTCSS_BW_ENABLE |
          (51U << BK4819_REG_51_SHIFT_CxCSS_TX_GAIN1));

  // CTC1 Frequency Control Word = 2775
  BK4819_WriteRegister(BK4819_REG_07,
                       0 | BK4819_REG_07_MODE_CTC1 |
                           (2775U << BK4819_REG_07_SHIFT_FREQUENCY));

  // Set the code word
  BK4819_WriteRegister(BK4819_REG_08, 0x0000 | ((CodeWord >> 0) & 0xFFF));
  BK4819_WriteRegister(BK4819_REG_08, 0x8000 | ((CodeWord >> 12) & 0xFFF));
}

void BK4819_SetCTCSSFrequency(uint32_t FreqControlWord) {
  uint16_t Config;

  if (FreqControlWord == 2625) { // Enables 1050Hz detection mode
    // Enable TxCTCSS
    // CTCSS Mode
    // 1050/4 Detect Enable
    // Enable Auto CDCSS Bw Mode
    // Enable Auto CTCSS Bw Mode
    // CTCSS/CDCSS Tx Gain1 Tuning = 74
    Config = 0x944A;
  } else {
    // Enable TxCTCSS
    // CTCSS Mode
    // Enable Auto CDCSS Bw Mode
    // Enable Auto CTCSS Bw Mode
    // CTCSS/CDCSS Tx Gain1 Tuning = 74
    Config = 0x904A;
  }
  BK4819_WriteRegister(BK4819_REG_51, Config);
  // CTC1 Frequency Control Word
  BK4819_WriteRegister(BK4819_REG_07, 0 | BK4819_REG_07_MODE_CTC1 |
                                          ((FreqControlWord * 2065) / 1000)
                                              << BK4819_REG_07_SHIFT_FREQUENCY);
}

void BK4819_SetTailDetection(const uint32_t freq_10Hz) {
  BK4819_WriteRegister(BK4819_REG_07,
                       BK4819_REG_07_MODE_CTC2 | ((253910 + (freq_10Hz / 2)) /
                                                  freq_10Hz)); // with rounding
}

void BK4819_EnableVox(uint16_t VoxEnableThreshold,
                      uint16_t VoxDisableThreshold) {
  // VOX Algorithm
  // if(voxamp>VoxEnableThreshold)       VOX = 1;
  // else if(voxamp<VoxDisableThreshold) (After Delay) VOX = 0;
  uint16_t REG_31_Value;

  REG_31_Value = BK4819_ReadRegister(BK4819_REG_31);
  // 0xA000 is undocumented?
  BK4819_WriteRegister(BK4819_REG_46, 0xA000 | (VoxEnableThreshold & 0x07FF));
  // 0x1800 is undocumented?
  BK4819_WriteRegister(BK4819_REG_79, 0x1800 | (VoxDisableThreshold & 0x07FF));
  // Bottom 12 bits are undocumented, 15:12 vox disable delay *128ms
  BK4819_WriteRegister(BK4819_REG_7A,
                       0x289A); // vox disable delay = 128*5 = 640ms
  // Enable VOX
  BK4819_WriteRegister(BK4819_REG_31, REG_31_Value | 4); // bit 2 - VOX Enable
}

void BK4819_SetFilterBandwidth(BK4819_FilterBandwidth_t Bandwidth) {
  /* if (BK4819_ReadRegister(BK4819_REG_43) !=
      BWRegValues[Bandwidth]) { // TODO: maybe slow */
  BK4819_WriteRegister(BK4819_REG_43, BWRegValues[Bandwidth]);
  // }
}

void BK4819_SetupPowerAmplifier(uint16_t Bias, uint32_t Frequency) {
  uint8_t Gain;

  if (Bias > 255) {
    Bias = 255;
  }
  if (Frequency < VHF_UHF_BOUND2) {
    // Gain 1 = 1
    // Gain 2 = 0
    Gain = 0x08U;
  } else {
    // Gain 1 = 4
    // Gain 2 = 2
    Gain = 0x22U;
  }
  // Enable PACTLoutput
  BK4819_WriteRegister(BK4819_REG_36, (Bias << 8) | 0x80U | Gain);
}

void BK4819_SetFrequency(uint32_t f) {
  BK4819_WriteRegister(BK4819_REG_38, f & 0xFFFF);
  BK4819_WriteRegister(BK4819_REG_39, (f >> 16) & 0xFFFF);
}

uint32_t BK4819_GetFrequency() {
  return (BK4819_ReadRegister(BK4819_REG_39) << 16) |
         BK4819_ReadRegister(BK4819_REG_38);
}

void BK4819_SetupSquelch(uint8_t SquelchOpenRSSIThresh,
                         uint8_t SquelchCloseRSSIThresh,
                         uint8_t SquelchOpenNoiseThresh,
                         uint8_t SquelchCloseNoiseThresh,
                         uint8_t SquelchCloseGlitchThresh,
                         uint8_t SquelchOpenGlitchThresh, uint8_t OpenDelay,
                         uint8_t CloseDelay) {
  BK4819_WriteRegister(BK4819_REG_70, 0);
  BK4819_WriteRegister(BK4819_REG_4D, 0xA000 | SquelchCloseGlitchThresh);
  BK4819_WriteRegister(
      BK4819_REG_4E,
      (1u << 14) |                      //  1 ???
          (uint16_t)(OpenDelay << 11) | // *5  squelch = open  delay .. 0 ~ 7
          (uint16_t)(CloseDelay << 9) | // *3  squelch = close delay .. 0 ~ 3
          SquelchOpenGlitchThresh);
  BK4819_WriteRegister(BK4819_REG_4F,
                       (SquelchCloseNoiseThresh << 8) | SquelchOpenNoiseThresh);
  BK4819_WriteRegister(BK4819_REG_78,
                       (SquelchOpenRSSIThresh << 8) | SquelchCloseRSSIThresh);
  BK4819_SetAF(BK4819_AF_MUTE);
  BK4819_RX_TurnOn();

  // NOTE: check if it works to prevent muting output
  // BK4819_SetAF(modTypeCurrent);
}

void BK4819_Squelch(uint8_t sql, uint32_t f, uint8_t OpenDelay,
                    uint8_t CloseDelay) {
  uint8_t band = f > SETTINGS_GetFilterBound() ? 1 : 0;
  BK4819_SetupSquelch(SQ[band][0][sql], SQ[band][1][sql], SQ[band][2][sql],
                      SQ[band][3][sql], SQ[band][4][sql], SQ[band][5][sql],
                      OpenDelay, CloseDelay);
}

void BK4819_SquelchType(SquelchType t) {
  const RegisterSpec sqType = {"SQ type", 0x77, 8, 0xFF, 1};
  const uint8_t squelchTypeValues[4] = {0x88, 0xAA, 0xCC, 0xFF};
  BK4819_SetRegValue(sqType, squelchTypeValues[t]);
}

void BK4819_SetAF(BK4819_AF_Type_t AF) {
  BK4819_WriteRegister(BK4819_REG_47, 0x6040 | (AF << 8));
}

uint16_t BK4819_GetRegValue(RegisterSpec s) {
  return (BK4819_ReadRegister(s.num) >> s.offset) & s.mask;
}

void BK4819_SetRegValue(RegisterSpec s, uint16_t v) {
  uint16_t reg = BK4819_ReadRegister(s.num);
  reg &= ~(s.mask << s.offset);
  BK4819_WriteRegister(s.num, reg | (v << s.offset));
}

void BK4819_SetModulation(ModulationType type) {
  if (modTypeCurrent == type) {
    return;
  }
  modTypeCurrent = type;
  const uint16_t modTypeReg47Values[] = {BK4819_AF_FM,  BK4819_AF_AM,
                                         BK4819_AF_USB, BK4819_AF_BYPASS,
                                         BK4819_AF_RAW, BK4819_AF_FM};
  BK4819_SetAF(modTypeReg47Values[type]);
  BK4819_SetRegValue(afDacGainRegSpec, 0xF);
  BK4819_SetAGC(type != MOD_AM);
  BK4819_WriteRegister(0x3D, type == MOD_USB ? 0 : 0x2AAB);
  BK4819_SetRegValue(afcDisableRegSpec,
                     type == MOD_AM || type == MOD_USB || type == MOD_BYP);
}

void BK4819_RX_TurnOn() {
  // DSP Voltage Setting = 1
  // ANA LDO = 2.7v
  // VCO LDO = 2.7v
  // RF LDO = 2.7v
  // PLL LDO = 2.7v
  // ANA LDO bypass
  // VCO LDO bypass
  // RF LDO bypass
  // PLL LDO bypass
  // Reserved bit is 1 instead of 0
  // Enable DSP
  // Enable XTAL
  // Enable Band Gap
  BK4819_WriteRegister(BK4819_REG_37, 0x1F0F);

  // Turn off everything
  BK4819_WriteRegister(BK4819_REG_30, 0);

  // Enable VCO Calibration
  // Enable RX Link
  // Enable AF DAC
  // Enable PLL/VCO
  // Disable PA Gain
  // Disable MIC ADC
  // Disable TX DSP
  // Enable RX DSP
  BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);
}

void BK4819_DisableFilter() {
  BK4819_ToggleGpioOut(BK4819_GPIO4_PIN32_VHF_LNA, false);
  BK4819_ToggleGpioOut(BK4819_GPIO3_PIN31_UHF_LNA, false);
}

void BK4819_SelectFilter(uint32_t f) {
  const Filter filterNeeded =
      f < SETTINGS_GetFilterBound() ? FILTER_VHF : FILTER_UHF;

  if (selectedFilter == filterNeeded) {
    return;
  }

  selectedFilter = filterNeeded;
  BK4819_ToggleGpioOut(BK4819_GPIO4_PIN32_VHF_LNA, filterNeeded == FILTER_VHF);
  BK4819_ToggleGpioOut(BK4819_GPIO3_PIN31_UHF_LNA, filterNeeded == FILTER_UHF);
}

void BK4819_DisableScramble() {
  uint16_t Value;

  Value = BK4819_ReadRegister(BK4819_REG_31);
  BK4819_WriteRegister(BK4819_REG_31, Value & 0xFFFD);
}

void BK4819_EnableScramble(uint8_t Type) {
  uint16_t Value;

  Value = BK4819_ReadRegister(BK4819_REG_31);
  BK4819_WriteRegister(BK4819_REG_31, Value | 2);
  BK4819_WriteRegister(BK4819_REG_71, (Type * 0x0408) + 0x68DC);
}

void BK4819_DisableVox() {
  uint16_t Value;

  Value = BK4819_ReadRegister(BK4819_REG_31);
  BK4819_WriteRegister(BK4819_REG_31, Value & 0xFFFB);
}

void BK4819_DisableDTMF() { BK4819_WriteRegister(BK4819_REG_24, 0); }

void BK4819_EnableDTMF() {
  BK4819_WriteRegister(BK4819_REG_21, 0x06D8);
  BK4819_WriteRegister(BK4819_REG_24,
                       0 | (1U << BK4819_REG_24_SHIFT_UNKNOWN_15) |
                           (24 << BK4819_REG_24_SHIFT_THRESHOLD) |
                           (1U << BK4819_REG_24_SHIFT_UNKNOWN_6) |
                           BK4819_REG_24_ENABLE | BK4819_REG_24_SELECT_DTMF |
                           (14U << BK4819_REG_24_SHIFT_MAX_SYMBOLS));
}

void BK4819_PlayTone(uint16_t Frequency, bool bTuningGainSwitch) {
  uint16_t ToneConfig;

  BK4819_EnterTxMute();
  BK4819_SetAF(BK4819_AF_BEEP);

  if (bTuningGainSwitch == 0) {
    ToneConfig = 0 | BK4819_REG_70_ENABLE_TONE1 |
                 (96U << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN);
  } else {
    ToneConfig = 0 | BK4819_REG_70_ENABLE_TONE1 |
                 (28U << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN);
  }
  BK4819_WriteRegister(BK4819_REG_70, ToneConfig);

  BK4819_WriteRegister(BK4819_REG_30, 0);
  BK4819_WriteRegister(BK4819_REG_30, 0 | BK4819_REG_30_ENABLE_AF_DAC |
                                          BK4819_REG_30_ENABLE_DISC_MODE |
                                          BK4819_REG_30_ENABLE_TX_DSP);

  BK4819_SetToneFrequency(Frequency);
}

void BK4819_EnterTxMute() { BK4819_WriteRegister(BK4819_REG_50, 0xBB20); }

void BK4819_ExitTxMute() { BK4819_WriteRegister(BK4819_REG_50, 0x3B20); }

void BK4819_Sleep() {
  BK4819_WriteRegister(BK4819_REG_30, 0);
  BK4819_WriteRegister(BK4819_REG_37, 0x1D00);
}

void BK4819_TurnsOffTones_TurnsOnRX() {
  BK4819_WriteRegister(BK4819_REG_70, 0);
  BK4819_SetAF(BK4819_AF_MUTE);
  BK4819_ExitTxMute();
  BK4819_WriteRegister(BK4819_REG_30, 0);
  BK4819_WriteRegister(
      BK4819_REG_30,
      0 | BK4819_REG_30_ENABLE_VCO_CALIB | BK4819_REG_30_ENABLE_RX_LINK |
          BK4819_REG_30_ENABLE_AF_DAC | BK4819_REG_30_ENABLE_DISC_MODE |
          BK4819_REG_30_ENABLE_PLL_VCO | BK4819_REG_30_ENABLE_RX_DSP);
}

void BK4819_ResetFSK() {
  BK4819_WriteRegister(BK4819_REG_3F, 0x0000); // Disable interrupts
  BK4819_WriteRegister(BK4819_REG_59,
                       0x0068); // Sync length 4 bytes, 7 byte preamble
  SYSTEM_DelayMs(30);
  BK4819_Idle();
}

void BK4819_FskClearFifo() {
  const uint16_t fsk_reg59 = BK4819_ReadRegister(BK4819_REG_59);
  BK4819_WriteRegister(BK4819_REG_59, (1u << 15) | (1u << 14) | fsk_reg59);
}

void BK4819_FskEnableRx() {
  const uint16_t fsk_reg59 = BK4819_ReadRegister(BK4819_REG_59);
  BK4819_WriteRegister(BK4819_REG_59, (1u << 12) | fsk_reg59);
}

void BK4819_FskEnableTx() {
  const uint16_t fsk_reg59 = BK4819_ReadRegister(BK4819_REG_59);
  BK4819_WriteRegister(BK4819_REG_59, (1u << 11) | fsk_reg59);
}

void BK4819_Idle() { BK4819_WriteRegister(BK4819_REG_30, 0x0000); }

void BK4819_ExitBypass() {
  BK4819_SetAF(BK4819_AF_MUTE);
  BK4819_WriteRegister(BK4819_REG_7E, 0x302E);
}

void BK4819_PrepareTransmit() {
  BK4819_ExitBypass();
  BK4819_ExitTxMute();
  BK4819_TxOn_Beep();
}

void BK4819_TxOn_Beep() {
  BK4819_WriteRegister(BK4819_REG_37, 0x1D0F);
  BK4819_WriteRegister(BK4819_REG_52, 0x028F);
  BK4819_WriteRegister(BK4819_REG_30, 0x0000);
  BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
}

void BK4819_ExitSubAu() { BK4819_WriteRegister(BK4819_REG_51, 0x0000); }

void BK4819_EnableRX() {
  if (gRxIdleMode) {
    BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, true);
    BK4819_RX_TurnOn();
  }
}

void BK4819_EnterDTMF_TX(bool bLocalLoopback) {
  BK4819_EnableDTMF();
  BK4819_EnterTxMute();
  if (bLocalLoopback) {
    BK4819_SetAF(BK4819_AF_BEEP);
  } else {
    BK4819_SetAF(BK4819_AF_MUTE);
  }
  BK4819_WriteRegister(BK4819_REG_70,
                       0 | BK4819_REG_70_MASK_ENABLE_TONE1 |
                           (83 << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN) |
                           BK4819_REG_70_MASK_ENABLE_TONE2 |
                           (83 << BK4819_REG_70_SHIFT_TONE2_TUNING_GAIN));

  BK4819_EnableTXLink();
}

void BK4819_ExitDTMF_TX(bool bKeep) {
  BK4819_EnterTxMute();
  BK4819_SetAF(BK4819_AF_MUTE);
  BK4819_WriteRegister(BK4819_REG_70, 0x0000);
  BK4819_DisableDTMF();
  BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
  if (!bKeep) {
    BK4819_ExitTxMute();
  }
}

void BK4819_EnableTXLink() {
  BK4819_WriteRegister(
      BK4819_REG_30,
      0 | BK4819_REG_30_ENABLE_VCO_CALIB | BK4819_REG_30_ENABLE_UNKNOWN |
          BK4819_REG_30_DISABLE_RX_LINK | BK4819_REG_30_ENABLE_AF_DAC |
          BK4819_REG_30_ENABLE_DISC_MODE | BK4819_REG_30_ENABLE_PLL_VCO |
          BK4819_REG_30_ENABLE_PA_GAIN | BK4819_REG_30_DISABLE_MIC_ADC |
          BK4819_REG_30_ENABLE_TX_DSP | BK4819_REG_30_DISABLE_RX_DSP);
}

void BK4819_PlayDTMF(char Code) {
  switch (Code) {
  case '0':
    BK4819_WriteRegister(BK4819_REG_71, 0x25F3);
    BK4819_WriteRegister(BK4819_REG_72, 0x35E1);
    break;
  case '1':
    BK4819_WriteRegister(BK4819_REG_71, 0x1C1C);
    BK4819_WriteRegister(BK4819_REG_72, 0x30C2);
    break;
  case '2':
    BK4819_WriteRegister(BK4819_REG_71, 0x1C1C);
    BK4819_WriteRegister(BK4819_REG_72, 0x35E1);
    break;
  case '3':
    BK4819_WriteRegister(BK4819_REG_71, 0x1C1C);
    BK4819_WriteRegister(BK4819_REG_72, 0x3B91);
    break;
  case '4':
    BK4819_WriteRegister(BK4819_REG_71, 0x1F0E);
    BK4819_WriteRegister(BK4819_REG_72, 0x30C2);
    break;
  case '5':
    BK4819_WriteRegister(BK4819_REG_71, 0x1F0E);
    BK4819_WriteRegister(BK4819_REG_72, 0x35E1);
    break;
  case '6':
    BK4819_WriteRegister(BK4819_REG_71, 0x1F0E);
    BK4819_WriteRegister(BK4819_REG_72, 0x3B91);
    break;
  case '7':
    BK4819_WriteRegister(BK4819_REG_71, 0x225C);
    BK4819_WriteRegister(BK4819_REG_72, 0x30C2);
    break;
  case '8':
    BK4819_WriteRegister(BK4819_REG_71, 0x225c);
    BK4819_WriteRegister(BK4819_REG_72, 0x35E1);
    break;
  case '9':
    BK4819_WriteRegister(BK4819_REG_71, 0x225C);
    BK4819_WriteRegister(BK4819_REG_72, 0x3B91);
    break;
  case 'A':
    BK4819_WriteRegister(BK4819_REG_71, 0x1C1C);
    BK4819_WriteRegister(BK4819_REG_72, 0x41DC);
    break;
  case 'B':
    BK4819_WriteRegister(BK4819_REG_71, 0x1F0E);
    BK4819_WriteRegister(BK4819_REG_72, 0x41DC);
    break;
  case 'C':
    BK4819_WriteRegister(BK4819_REG_71, 0x225C);
    BK4819_WriteRegister(BK4819_REG_72, 0x41DC);
    break;
  case 'D':
    BK4819_WriteRegister(BK4819_REG_71, 0x25F3);
    BK4819_WriteRegister(BK4819_REG_72, 0x41DC);
    break;
  case '*':
    BK4819_WriteRegister(BK4819_REG_71, 0x25F3);
    BK4819_WriteRegister(BK4819_REG_72, 0x30C2);
    break;
  case '#':
    BK4819_WriteRegister(BK4819_REG_71, 0x25F3);
    BK4819_WriteRegister(BK4819_REG_72, 0x3B91);
    break;
  }
}

void BK4819_PlayDTMFString(const char *pString, bool bDelayFirst,
                           uint16_t FirstCodePersistTime,
                           uint16_t HashCodePersistTime,
                           uint16_t CodePersistTime,
                           uint16_t CodeInternalTime) {
  uint8_t i;
  uint16_t Delay;

  for (i = 0; pString[i]; i++) {
    BK4819_PlayDTMF(pString[i]);
    BK4819_ExitTxMute();
    if (bDelayFirst && i == 0) {
      Delay = FirstCodePersistTime;
    } else if (pString[i] == '*' || pString[i] == '#') {
      Delay = HashCodePersistTime;
    } else {
      Delay = CodePersistTime;
    }
    SYSTEM_DelayMs(Delay);
    BK4819_EnterTxMute();
    SYSTEM_DelayMs(CodeInternalTime);
  }
}

void BK4819_TransmitTone(bool bLocalLoopback, uint32_t Frequency) {
  BK4819_EnterTxMute();
  BK4819_WriteRegister(BK4819_REG_70,
                       BK4819_REG_70_MASK_ENABLE_TONE1 |
                           (96U << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
  BK4819_SetToneFrequency(Frequency);

  BK4819_SetAF(bLocalLoopback ? BK4819_AF_BEEP : BK4819_AF_MUTE);

  BK4819_EnableTXLink();
  SYSTEM_DelayMs(50);
  BK4819_ExitTxMute();
}

void BK4819_GenTail(uint8_t Tail) {
  switch (Tail) {
  case 0: // 134.4Hz CTCSS Tail
    BK4819_WriteRegister(BK4819_REG_52, 0x828F);
    break;
  case 1: // 120° phase shift
    BK4819_WriteRegister(BK4819_REG_52, 0xA28F);
    break;
  case 2: // 180° phase shift
    BK4819_WriteRegister(BK4819_REG_52, 0xC28F);
    break;
  case 3: // 240° phase shift
    BK4819_WriteRegister(BK4819_REG_52, 0xE28F);
    break;
  case 4: // 55Hz tone freq NOTE: REG_07
    BK4819_WriteRegister(BK4819_REG_07, 0x046f);
    break;
  }
}

void BK4819_EnableCDCSS() {
  BK4819_GenTail(0); // CTC134
  BK4819_WriteRegister(BK4819_REG_51, 0x804A);
}

void BK4819_EnableCTCSS() {
  // BK4819_GenTail(2); // CTC180
  BK4819_GenTail(4); // CTC55
  BK4819_WriteRegister(BK4819_REG_51, 0x904A);
}

uint16_t BK4819_GetRSSI() {
  return BK4819_ReadRegister(BK4819_REG_67) & 0x1FF;
}

uint8_t BK4819_GetNoise() {
  return BK4819_ReadRegister(BK4819_REG_65) & 0x7F;
}

uint8_t BK4819_GetGlitch() {
  return BK4819_ReadRegister(BK4819_REG_63) & 0xFF;
}

uint8_t BK4819_GetSNR() { return (BK4819_ReadRegister(0x61) >> 8) & 0xFF; }

bool BK4819_GetFrequencyScanResult(uint32_t *pFrequency) {
  uint16_t High = BK4819_ReadRegister(BK4819_REG_0D);
  bool Finished = (High & 0x8000) == 0;

  if (Finished) {
    uint16_t Low = BK4819_ReadRegister(BK4819_REG_0E);
    *pFrequency = (uint32_t)((High & 0x7FF) << 16) | Low;
  }

  return Finished;
}

BK4819_CssScanResult_t BK4819_GetCxCSSScanResult(uint32_t *pCdcssFreq,
                                                 uint16_t *pCtcssFreq) {
  uint16_t Low;
  uint16_t High = BK4819_ReadRegister(BK4819_REG_69);

  if ((High & 0x8000) == 0) {
    Low = BK4819_ReadRegister(BK4819_REG_6A);
    *pCdcssFreq = ((High & 0xFFF) << 12) | (Low & 0xFFF);
    return BK4819_CSS_RESULT_CDCSS;
  }

  Low = BK4819_ReadRegister(BK4819_REG_68);

  if ((Low & 0x8000) == 0) {
    *pCtcssFreq = ((Low & 0x1FFF) * 4843) / 10000;
    return BK4819_CSS_RESULT_CTCSS;
  }

  return BK4819_CSS_RESULT_NOT_FOUND;
}

void BK4819_DisableFrequencyScan() {
  BK4819_WriteRegister(BK4819_REG_32, 0x0244);
}

void BK4819_EnableFrequencyScan() {
  BK4819_WriteRegister(BK4819_REG_32, 0x0245);
}

void BK4819_EnableFrequencyScanEx(FreqScanTime t) {
  BK4819_WriteRegister(BK4819_REG_32, 0x0245 | (t << 14));
}

void BK4819_SetScanFrequency(uint32_t Frequency) {
  BK4819_SetFrequency(Frequency);
  BK4819_WriteRegister(
      BK4819_REG_51,
      0 | BK4819_REG_51_DISABLE_CxCSS | BK4819_REG_51_GPIO6_PIN2_NORMAL |
          BK4819_REG_51_TX_CDCSS_POSITIVE | BK4819_REG_51_MODE_CDCSS |
          BK4819_REG_51_CDCSS_23_BIT | BK4819_REG_51_1050HZ_NO_DETECTION |
          BK4819_REG_51_AUTO_CDCSS_BW_DISABLE |
          BK4819_REG_51_AUTO_CTCSS_BW_DISABLE);
  BK4819_RX_TurnOn();
}

void BK4819_StopScan() {
  BK4819_DisableFrequencyScan();
  BK4819_Idle();
}

uint8_t BK4819_GetDTMF_5TONE_Code() {
  return (BK4819_ReadRegister(BK4819_REG_0B) >> 8) & 0x0F;
}

uint8_t BK4819_GetCDCSSCodeType() {
  return (BK4819_ReadRegister(BK4819_REG_0C) >> 14) & 3;
}

uint8_t BK4819_GetCTCType() {
  return (BK4819_ReadRegister(BK4819_REG_0C) >> 10) & 3;
}

#if defined(ENABLE_AIRCOPY)
void BK4819_SendFSKData(uint16_t *pData) {
  uint8_t i;
  uint8_t Timeout = 200;

  SYSTEM_DelayMs(20);

  BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_FSK_TX_FINISHED);
  BK4819_WriteRegister(BK4819_REG_59, 0x8068);
  BK4819_WriteRegister(BK4819_REG_59, 0x0068);

  for (i = 0; i < 36; i++) {
    BK4819_WriteRegister(BK4819_REG_5F, pData[i]);
  }

  SYSTEM_DelayMs(20);

  BK4819_WriteRegister(BK4819_REG_59, 0x2868);

  while (Timeout) {
    if (BK4819_ReadRegister(BK4819_REG_0C) & 1U) {
      break;
    }
    SYSTEM_DelayMs(5);
    Timeout--;
  }

  BK4819_WriteRegister(BK4819_REG_02, 0);
  SYSTEM_DelayMs(20);
  BK4819_ResetFSK();
}

void BK4819_PrepareFSKReceive() {
  BK4819_ResetFSK();
  BK4819_WriteRegister(BK4819_REG_02, 0);
  BK4819_WriteRegister(BK4819_REG_3F, 0);
  BK4819_RX_TurnOn();
  BK4819_WriteRegister(BK4819_REG_3F, 0 | BK4819_REG_3F_FSK_RX_FINISHED |
                                          BK4819_REG_3F_FSK_FIFO_ALMOST_FULL);
  // Clear RX FIFO
  // FSK Preamble Length 7 bytes
  // FSK SyncLength Selection
  BK4819_WriteRegister(BK4819_REG_59, 0x4068);
  // Enable FSK Scramble
  // Enable FSK RX
  // FSK Preamble Length 7 bytes
  // FSK SyncLength Selection
  BK4819_WriteRegister(BK4819_REG_59, 0x3068);
}
#endif

void BK4819_PlayRoger() {
  BK4819_EnterTxMute();
  BK4819_SetAF(BK4819_AF_MUTE);
  BK4819_WriteRegister(BK4819_REG_70, 0xE000);
  BK4819_EnableTXLink();
  SYSTEM_DelayMs(50);
  BK4819_WriteRegister(BK4819_REG_71, 0x142A);
  BK4819_ExitTxMute();
  SYSTEM_DelayMs(80);
  BK4819_EnterTxMute();
  BK4819_WriteRegister(BK4819_REG_71, 0x1C3B);
  BK4819_ExitTxMute();
  SYSTEM_DelayMs(80);
  BK4819_EnterTxMute();
  BK4819_WriteRegister(BK4819_REG_70, 0x0000);
  BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
}

void BK4819_PlayRogerMDC() {
  uint8_t i;

  BK4819_SetAF(BK4819_AF_MUTE);
  BK4819_WriteRegister(
      BK4819_REG_58,
      0x37C3); // FSK Enable, RX Bandwidth FFSK1200/1800, 0xAA or 0x55 Preamble,
               // 11 RX Gain, 101 RX Mode, FFSK1200/1800 TX
  BK4819_WriteRegister(BK4819_REG_72, 0x3065); // Set Tone2 to 1200Hz
  BK4819_WriteRegister(BK4819_REG_70,
                       0x00E0); // Enable Tone2 and Set Tone2 Gain
  BK4819_WriteRegister(BK4819_REG_5D,
                       0x0D00); // Set FSK data length to 13 bytes
  BK4819_WriteRegister(
      BK4819_REG_59,
      0x8068); // 4 byte sync length, 6 byte preamble, clear TX FIFO
  BK4819_WriteRegister(
      BK4819_REG_59,
      0x0068); // Same, but clear TX FIFO is now unset (clearing done)
  BK4819_WriteRegister(BK4819_REG_5A, 0x5555); // First two sync bytes
  BK4819_WriteRegister(BK4819_REG_5B,
                       0x55AA); // End of sync bytes. Total 4 bytes: 555555aa
  BK4819_WriteRegister(BK4819_REG_5C, 0xAA30); // Disable CRC
  for (i = 0; i < 7; i++) {
    BK4819_WriteRegister(
        BK4819_REG_5F, FSK_RogerTable[i]); // Send the data from the roger table
  }
  SYSTEM_DelayMs(20);
  BK4819_WriteRegister(BK4819_REG_59,
                       0x0868); // 4 sync bytes, 6 byte preamble, Enable FSK TX
  SYSTEM_DelayMs(180);
  // Stop FSK TX, reset Tone2, disable FSK.
  BK4819_WriteRegister(BK4819_REG_59, 0x0068);
  BK4819_WriteRegister(BK4819_REG_70, 0x0000);
  BK4819_WriteRegister(BK4819_REG_58, 0x0000);
}

void BK4819_Enable_AfDac_DiscMode_TxDsp() {
  BK4819_WriteRegister(BK4819_REG_30, 0x0000);
  BK4819_WriteRegister(BK4819_REG_30, 0x0302);
}

void BK4819_GetVoxAmp(uint16_t *pResult) {
  *pResult = BK4819_ReadRegister(BK4819_REG_64) & 0x7FFF;
}

void BK4819_PlayDTMFEx(bool bLocalLoopback, char Code) {
  BK4819_EnableDTMF();
  BK4819_EnterTxMute();
  BK4819_SetAF(bLocalLoopback ? BK4819_AF_BEEP : BK4819_AF_MUTE);
  BK4819_WriteRegister(BK4819_REG_70, 0xD3D3);
  BK4819_EnableTXLink();
  SYSTEM_DelayMs(50);
  BK4819_PlayDTMF(Code);
  BK4819_ExitTxMute();
}

void BK4819_ToggleAFBit(bool on) {
  uint16_t reg = BK4819_ReadRegister(BK4819_REG_47);
  reg &= ~(1 << 8);
  if (on)
    reg |= 1 << 8;
  BK4819_WriteRegister(BK4819_REG_47, reg);
}

void BK4819_ToggleAFDAC(bool on) {
  uint16_t Reg = BK4819_ReadRegister(BK4819_REG_30);
  Reg &= ~BK4819_REG_30_ENABLE_AF_DAC;
  if (on)
    Reg |= BK4819_REG_30_ENABLE_AF_DAC;
  BK4819_WriteRegister(BK4819_REG_30, Reg);
}

void BK4819_TuneTo(uint32_t f, bool precise) {
  BK4819_SelectFilter(f);
  BK4819_SetFrequency(f);
  uint16_t reg = BK4819_ReadRegister(BK4819_REG_30);
  if (precise) {
    BK4819_WriteRegister(BK4819_REG_30, 0);
  } else {
    BK4819_WriteRegister(BK4819_REG_30, reg & ~BK4819_REG_30_ENABLE_VCO_CALIB);
  }
  BK4819_WriteRegister(BK4819_REG_30, reg);
}

void BK4819_SetToneFrequency(uint16_t f) {
  BK4819_WriteRegister(BK4819_REG_71, (f * 103U) / 10U);
}

bool BK4819_IsSquelchOpen() {
  return (BK4819_ReadRegister(BK4819_REG_0C) >> 1) & 1;
}

void BK4819_ResetRSSI() {
  uint16_t Reg = BK4819_ReadRegister(BK4819_REG_30);
  Reg &= ~1;
  BK4819_WriteRegister(BK4819_REG_30, Reg);
  Reg |= 1;
  BK4819_WriteRegister(BK4819_REG_30, Reg);
}

void BK4819_SetGain(uint8_t gainIndex) {
  BK4819_WriteRegister(BK4819_REG_13,
                       gainTable[gainIndex].regValue | 6 | (3 << 3));
}

void BK4819_HandleInterrupts(void (*handler)(uint16_t intStatus)) {
  while (BK4819_ReadRegister(BK4819_REG_0C) & 1u) {
    BK4819_WriteRegister(BK4819_REG_02, 0);
    handler(BK4819_ReadRegister(BK4819_REG_02));
  }
}
