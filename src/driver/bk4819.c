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

bool gRxIdleMode;

const uint8_t DTMF_COEFFS[] = {111, 107, 103, 98, 80,  71,  58,  44,
                               65,  55,  37,  23, 228, 203, 181, 159};

const uint8_t SQ[2][6][11] = {
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
};

const uint16_t BWRegValues[3] = {0x3028, 0x4048, 0x0018};

const Gain gainTable[96] = {
    {0x0000, -98}, //   1 .. 0 0 0 0 .. -33dB -24dB -8dB -33dB .. -98dB
    {0x0008, -96}, //   2 .. 0 0 1 0 .. -33dB -24dB -6dB -33dB .. -96dB
    {0x0100, -95}, //   3 .. 1 0 0 0 .. -30dB -24dB -8dB -33dB .. -95dB
    {0x0020, -93}, //   4 .. 0 1 0 0 .. -33dB -19dB -8dB -33dB .. -93dB
    {0x0001, -92}, //   5 .. 0 0 0 1 .. -33dB -24dB -8dB -27dB .. -92dB
    {0x0028, -91}, //   6 .. 0 1 1 0 .. -33dB -19dB -6dB -33dB .. -91dB
    {0x0009, -90}, //   7 .. 0 0 1 1 .. -33dB -24dB -6dB -27dB .. -90dB
    {0x0101, -89}, //   8 .. 1 0 0 1 .. -30dB -24dB -8dB -27dB .. -89dB
    {0x0030, -88}, //   9 .. 0 1 2 0 .. -33dB -19dB -3dB -33dB .. -88dB
    {0x0118, -87}, //  10 .. 1 0 3 0 .. -30dB -24dB  0dB -33dB .. -87dB
    {0x0002, -86}, //  11 .. 0 0 0 2 .. -33dB -24dB -8dB -21dB .. -86dB
    {0x0130, -85}, //  12 .. 1 1 2 0 .. -30dB -19dB -3dB -33dB .. -85dB
    {0x0019, -84}, //  13 .. 0 0 3 1 .. -33dB -24dB  0dB -27dB .. -84dB
    {0x0060, -83}, //  14 .. 0 3 0 0 .. -33dB  -9dB -8dB -33dB .. -83dB
    {0x0138, -82}, //  15 .. 1 1 3 0 .. -30dB -19dB  0dB -33dB .. -82dB
    {0x0119, -81}, //  16 .. 1 0 3 1 .. -30dB -24dB  0dB -27dB .. -81dB
    {0x0058, -80}, //  17 .. 0 2 3 0 .. -33dB -14dB  0dB -33dB .. -80dB
    {0x0141, -79}, //  18 .. 1 2 0 1 .. -30dB -14dB -8dB -27dB .. -79dB
    {0x0070, -78}, //  19 .. 0 3 2 0 .. -33dB  -9dB -3dB -33dB .. -78dB
    {0x0180, -77}, //  20 .. 1 4 0 0 .. -30dB  -6dB -8dB -33dB .. -77dB
    {0x0139, -76}, //  21 .. 1 1 3 1 .. -30dB -19dB  0dB -27dB .. -76dB
    {0x0013, -75}, //  22 .. 0 0 2 3 .. -33dB -24dB -3dB -15dB .. -75dB
    {0x0161, -74}, //  23 .. 1 3 0 1 .. -30dB  -9dB -8dB -27dB .. -74dB
    {0x01C0, -73}, //  24 .. 1 6 0 0 .. -30dB  -2dB -8dB -33dB .. -73dB
    {0x00E8, -72}, //  25 .. 0 7 1 0 .. -33dB   0dB -6dB -33dB .. -72dB
    {0x00D0, -71}, //  26 .. 0 6 2 0 .. -33dB  -2dB -3dB -33dB .. -71dB
    {0x0239, -70}, //  27 .. 2 1 3 1 .. -24dB -19dB  0dB -27dB .. -70dB
    {0x006A, -69}, //  28 .. 0 3 1 2 .. -33dB  -9dB -6dB -21dB .. -69dB
    {0x0006, -68}, //  29 .. 0 0 0 6 .. -33dB -24dB -8dB  -3dB .. -68dB
    {0x00B1, -67}, //  30 .. 0 5 2 1 .. -33dB  -4dB -3dB -27dB .. -67dB
    {0x000E, -66}, //  31 .. 0 0 1 6 .. -33dB -24dB -6dB  -3dB .. -66dB
    {0x015A, -65}, //  32 .. 1 2 3 2 .. -30dB -14dB  0dB -21dB .. -65dB
    {0x022B, -64}, //  33 .. 2 1 1 3 .. -24dB -19dB -6dB -15dB .. -64dB
    {0x01F8, -63}, //  34 .. 1 7 3 0 .. -30dB   0dB  0dB -33dB .. -63dB
    {0x0163, -62}, //  35 .. 1 3 0 3 .. -30dB  -9dB -8dB -15dB .. -62dB
    {0x0035, -61}, //  36 .. 0 1 2 5 .. -33dB -19dB -3dB  -6dB .. -61dB
    {0x0214, -60}, //  37 .. 2 0 2 4 .. -24dB -24dB -3dB  -9dB .. -60dB
    {0x01D9, -59}, //  38 .. 1 6 3 1 .. -30dB  -2dB  0dB -27dB .. -59dB
    {0x0145, -58}, //  39 .. 1 2 0 5 .. -30dB -14dB -8dB  -6dB .. -58dB
    {0x02A2, -57}, //  40 .. 2 5 0 2 .. -24dB  -4dB -8dB -21dB .. -57dB
    {0x02D1, -56}, //  41 .. 2 6 2 1 .. -24dB  -2dB -3dB -27dB .. -56dB
    {0x00B3, -55}, //  42 .. 0 5 2 3 .. -33dB  -4dB -3dB -15dB .. -55dB
    {0x0216, -54}, //  43 .. 2 0 2 6 .. -24dB -24dB -3dB  -3dB .. -54dB
    {0x0066, -53}, //  44 .. 0 3 0 6 .. -33dB  -9dB -8dB  -3dB .. -53dB
    {0x00C4, -52}, //  45 .. 0 6 0 4 .. -33dB  -2dB -8dB  -9dB .. -52dB
    {0x006E, -51}, //  46 .. 0 3 1 6 .. -33dB  -9dB -6dB  -3dB .. -51dB
    {0x015D, -50}, //  47 .. 1 2 3 5 .. -30dB -14dB  0dB  -6dB .. -50dB
    {0x00AD, -49}, //  48 .. 0 5 1 5 .. -33dB  -4dB -6dB  -6dB .. -49dB
    {0x007D, -48}, //  49 .. 0 3 3 5 .. -33dB  -9dB  0dB  -6dB .. -48dB
    {0x00D4, -47}, //  50 .. 0 6 2 4 .. -33dB  -2dB -3dB  -9dB .. -47dB
    {0x01B4, -46}, //  51 .. 1 5 2 4 .. -30dB  -4dB -3dB  -9dB .. -46dB
    {0x030B, -45}, //  52 .. 3 0 1 3 ..   0dB -24dB -6dB -15dB .. -45dB
    {0x00CE, -44}, //  53 .. 0 6 1 6 .. -33dB  -2dB -6dB  -3dB .. -44dB
    {0x01B5, -43}, //  54 .. 1 5 2 5 .. -30dB  -4dB -3dB  -6dB .. -43dB
    {0x0097, -42}, //  55 .. 0 4 2 7 .. -33dB  -6dB -3dB   0dB .. -42dB
    {0x0257, -41}, //  56 .. 2 2 2 7 .. -24dB -14dB -3dB   0dB .. -41dB
    {0x02B4, -40}, //  57 .. 2 5 2 4 .. -24dB  -4dB -3dB  -9dB .. -40dB
    {0x027D, -39}, //  58 .. 2 3 3 5 .. -24dB  -9dB  0dB  -6dB .. -39dB
    {0x01DD, -38}, //  59 .. 1 6 3 5 .. -30dB  -2dB  0dB  -6dB .. -38dB
    {0x02AE, -37}, //  60 .. 2 5 1 6 .. -24dB  -4dB -6dB  -3dB .. -37dB
    {0x0379, -36}, //  61 .. 3 3 3 1 ..   0dB  -9dB  0dB -27dB .. -36dB
    {0x035A, -35}, //  62 .. 3 2 3 2 ..   0dB -14dB  0dB -21dB .. -35dB
    {0x02B6, -34}, //  63 .. 2 5 2 6 .. -24dB  -4dB -3dB  -3dB .. -34dB
    {0x030E, -33}, //  64 .. 3 0 1 6 ..   0dB -24dB -6dB  -3dB .. -33dB
    {0x0307, -32}, //  65 .. 3 0 0 7 ..   0dB -24dB -8dB   0dB .. -32dB
    {0x02BE, -31}, //  66 .. 2 5 3 6 .. -24dB  -4dB  0dB  -3dB .. -31dB
    {0x037A, -30}, //  67 .. 3 3 3 2 ..   0dB  -9dB  0dB -21dB .. -30dB
    {0x02DE, -29}, //  68 .. 2 6 3 6 .. -24dB  -2dB  0dB  -3dB .. -29dB
    {0x0345, -28}, //  69 .. 3 2 0 5 ..   0dB -14dB -8dB  -6dB .. -28dB
    {0x03A3, -27}, //  70 .. 3 5 0 3 ..   0dB  -4dB -8dB -15dB .. -27dB
    {0x0364, -26}, //  71 .. 3 3 0 4 ..   0dB  -9dB -8dB  -9dB .. -26dB
    {0x032F, -25}, //  72 .. 3 1 1 7 ..   0dB -19dB -6dB   0dB .. -25dB
    {0x0393, -24}, //  73 .. 3 4 2 3 ..   0dB  -6dB -3dB -15dB .. -24dB
    {0x0384, -23}, //  74 .. 3 4 0 4 ..   0dB  -6dB -8dB  -9dB .. -23dB
    {0x0347, -22}, //  75 .. 3 2 0 7 ..   0dB -14dB -8dB   0dB .. -22dB
    {0x03EB, -21}, //  76 .. 3 7 1 3 ..   0dB   0dB -6dB -15dB .. -21dB
    {0x03D3, -20}, //  77 .. 3 6 2 3 ..   0dB  -2dB -3dB -15dB .. -20dB
    {0x03BB, -19}, //  78 .. 3 5 3 3 ..   0dB  -4dB  0dB -15dB .. -19dB
    {0x037C, -18}, //  79 .. 3 3 3 4 ..   0dB  -9dB  0dB  -9dB .. -18dB
    {0x03CC, -17}, //  80 .. 3 6 1 4 ..   0dB  -2dB -6dB  -9dB .. -17dB
    {0x03C5, -16}, //  81 .. 3 6 0 5 ..   0dB  -2dB -8dB  -6dB .. -16dB
    {0x03EC, -15}, //  82 .. 3 7 1 4 ..   0dB   0dB -6dB  -9dB .. -15dB
    {0x035F, -14}, //  83 .. 3 2 3 7 ..   0dB -14dB  0dB   0dB .. -14dB
    {0x03BC, -13}, //  84 .. 3 5 3 4 ..   0dB  -4dB  0dB  -9dB .. -13dB
    {0x038F, -12}, //  85 .. 3 4 1 7 ..   0dB  -6dB -6dB   0dB .. -12dB
    {0x03E6, -11}, //  86 .. 3 7 0 6 ..   0dB   0dB -8dB  -3dB .. -11dB
    {0x03AF, -10}, //  87 .. 3 5 1 7 ..   0dB  -4dB -6dB   0dB .. -10dB
    {0x03F5, -9},  //  88 .. 3 7 2 5 ..   0dB   0dB -3dB  -6dB ..  -9dB
    {0x03D6, -8},  //  89 .. 3 6 2 6 ..   0dB  -2dB -3dB  -3dB ..  -8dB
    {0x03BE, -7},  //  90 .. 3 5 3 6 ..   0dB  -4dB  0dB  -3dB ..  -7dB original
    {0x03F6, -6},  //  91 .. 3 7 2 6 ..   0dB   0dB -3dB  -3dB ..  -6dB
    {0x03DE, -5},  //  92 .. 3 6 3 6 ..   0dB  -2dB  0dB  -3dB ..  -5dB
    {0x03BF, -4},  //  93 .. 3 5 3 7 ..   0dB  -4dB  0dB   0dB ..  -4dB
    {0x03F7, -3},  //  94 .. 3 7 2 7 ..   0dB   0dB -3dB   0dB ..  -3dB
    {0x03DF, -2},  //  95 .. 3 6 3 7 ..   0dB  -2dB  0dB   0dB ..  -2dB
    {0x03FF, 0},   //  96 .. 3 7 3 7 ..   0dB   0dB  0dB   0dB ..   0dB
};

void BK4819_Init(void) {
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

static uint16_t BK4819_ReadU16(void) {
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
  BK4819_WriteRegister(BK4819_REG_13, 0x03BE);
  BK4819_WriteRegister(BK4819_REG_12, 0x037B);
  BK4819_WriteRegister(BK4819_REG_11, 0x027B);
  BK4819_WriteRegister(BK4819_REG_10, 0x007A);

  uint8_t Lo = 0;    // 0-1 - auto, 2 - low, 3 high
  uint8_t low = 56;  // 1dB / LSB
  uint8_t high = 84; // 1dB / LSB

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
  UART_printf("BK4819_SetFilterBandwidth(%u)\n", Bandwidth);
  UART_flush();
  if (BK4819_ReadRegister(BK4819_REG_43) !=
      BWRegValues[Bandwidth]) { // TODO: maybe slow
    BK4819_WriteRegister(BK4819_REG_43, BWRegValues[Bandwidth]);
  }
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
  UART_printf("BK4819_SetFrequency(%u)\n", f);
  UART_flush();
  BK4819_WriteRegister(BK4819_REG_38, f & 0xFFFF);
  BK4819_WriteRegister(BK4819_REG_39, (f >> 16) & 0xFFFF);
}

uint32_t BK4819_GetFrequency(void) {
  return (BK4819_ReadRegister(BK4819_REG_39) << 16) |
         BK4819_ReadRegister(BK4819_REG_38);
}

void BK4819_SetupSquelch(uint8_t SquelchOpenRSSIThresh,
                         uint8_t SquelchCloseRSSIThresh,
                         uint8_t SquelchOpenNoiseThresh,
                         uint8_t SquelchCloseNoiseThresh,
                         uint8_t SquelchCloseGlitchThresh,
                         uint8_t SquelchOpenGlitchThresh) {
  BK4819_WriteRegister(BK4819_REG_70, 0);
  BK4819_WriteRegister(BK4819_REG_4D, 0xA000 | SquelchCloseGlitchThresh);
  BK4819_WriteRegister(BK4819_REG_4E,
                       (1u << 14) |     //  1 ???
                           (0u << 11) | // *5  squelch = open  delay .. 0 ~ 7
                           (0u << 9) |  // *3  squelch = close delay .. 0 ~ 3
                           SquelchOpenGlitchThresh);
  BK4819_WriteRegister(BK4819_REG_4F,
                       (SquelchCloseNoiseThresh << 8) | SquelchOpenNoiseThresh);
  BK4819_WriteRegister(BK4819_REG_78,
                       (SquelchOpenRSSIThresh << 8) | SquelchCloseRSSIThresh);
  BK4819_SetAF(BK4819_AF_MUTE);
  BK4819_RX_TurnOn();
}

void BK4819_Squelch(uint8_t sql, uint32_t f) {
  uint8_t band = f > SETTINGS_GetFilterBound() ? 1 : 0;
  UART_printf("BK4819_Squelch(%u, %u): %u\n", sql, f, band);
  UART_flush();
  BK4819_SetupSquelch(SQ[band][0][sql], SQ[band][1][sql], SQ[band][2][sql],
                      SQ[band][3][sql], SQ[band][4][sql], SQ[band][5][sql]);
}

void BK4819_SquelchType(SquelchType t) {
  const RegisterSpec sqType = {"SQ type", 0x77, 8, 0xFF, 1};
  const uint8_t squelchTypeValues[4] = {0x88, 0xAA, 0xCC, 0xFF};
  BK4819_SetRegValue(sqType, squelchTypeValues[t]);
}

void BK4819_SetAF(BK4819_AF_Type_t AF) {
  UART_printf("BK4819_SetAF(%u)\n", AF);
  UART_flush();
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

static uint8_t modTypeCurrent = 255;

void BK4819_SetModulation(ModulationType type) {
  if (modTypeCurrent == type) {
    return;
  }
  modTypeCurrent = type;
  UART_printf("BK4819_SetModulation(%u)\n", type);
  UART_flush();
  const uint16_t modTypeReg47Values[] = {BK4819_AF_FM,  BK4819_AF_AM,
                                         BK4819_AF_USB, BK4819_AF_BYPASS,
                                         BK4819_AF_RAW, BK4819_AF_FM};
  BK4819_SetAF(modTypeReg47Values[type]);
  BK4819_SetRegValue(afDacGainRegSpec, 0xF);
  BK4819_SetAGC(type != MOD_AM);
  BK4819_WriteRegister(0x3D, type == MOD_USB ? 0 : 0x2AAB);
  BK4819_SetRegValue(afcDisableRegSpec, type != MOD_FM);
}

void BK4819_RX_TurnOn(void) {
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

void BK4819_DisableFilter(void) {
  BK4819_ToggleGpioOut(BK4819_GPIO4_PIN32_VHF_LNA, false);
  BK4819_ToggleGpioOut(BK4819_GPIO3_PIN31_UHF_LNA, false);
}

void BK4819_SelectFilter(uint32_t f) {
  Filter filterNeeded = f < SETTINGS_GetFilterBound() ? FILTER_VHF : FILTER_UHF;

  if (selectedFilter == filterNeeded) {
    return;
  }

  UART_printf("BK4819_SelectFilter(%u): %u\n", f, filterNeeded);
  UART_flush();

  selectedFilter = filterNeeded;
  BK4819_ToggleGpioOut(BK4819_GPIO4_PIN32_VHF_LNA, filterNeeded == FILTER_VHF);
  BK4819_ToggleGpioOut(BK4819_GPIO3_PIN31_UHF_LNA, filterNeeded == FILTER_UHF);
}

void BK4819_DisableScramble(void) {
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

void BK4819_DisableVox(void) {
  uint16_t Value;

  Value = BK4819_ReadRegister(BK4819_REG_31);
  BK4819_WriteRegister(BK4819_REG_31, Value & 0xFFFB);
}

void BK4819_DisableDTMF(void) { BK4819_WriteRegister(BK4819_REG_24, 0); }

void BK4819_EnableDTMF(void) {
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

void BK4819_EnterTxMute(void) { BK4819_WriteRegister(BK4819_REG_50, 0xBB20); }

void BK4819_ExitTxMute(void) { BK4819_WriteRegister(BK4819_REG_50, 0x3B20); }

void BK4819_Sleep(void) {
  BK4819_WriteRegister(BK4819_REG_30, 0);
  BK4819_WriteRegister(BK4819_REG_37, 0x1D00);
}

void BK4819_TurnsOffTones_TurnsOnRX(void) {
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

#if defined(ENABLE_AIRCOPY)
void BK4819_SetupAircopy(void) {
  BK4819_WriteRegister(BK4819_REG_70, 0x00E0); // Enable Tone2, tuning gain 48
  BK4819_WriteRegister(BK4819_REG_72, 0x3065); // Tone2 baudrate 1200
  BK4819_WriteRegister(
      BK4819_REG_58,
      0x00C1); // FSK Enable, FSK 1.2K RX Bandwidth, Preamble 0xAA or 0x55, RX
               // Gain 0, RX Mode (FSK1.2K, FSK2.4K Rx and NOAA SAME Rx), TX
               // Mode FSK 1.2K and FSK 2.4K Tx
  BK4819_WriteRegister(
      BK4819_REG_5C, 0x5665); // Enable CRC among other things we don't know yet
  BK4819_WriteRegister(
      BK4819_REG_5D, 0x4700); // FSK Data Length 72 Bytes (0xabcd + 2 byte
                              // length + 64 byte payload + 2 byte CRC + 0xdcba)
}

void BK4819_ResetFSK(void) {
  BK4819_WriteRegister(BK4819_REG_3F, 0x0000); // Disable interrupts
  BK4819_WriteRegister(BK4819_REG_59,
                       0x0068); // Sync length 4 bytes, 7 byte preamble
  SYSTEM_DelayMs(30);
  BK4819_Idle();
}
#endif

void BK4819_Idle(void) { BK4819_WriteRegister(BK4819_REG_30, 0x0000); }

void BK4819_ExitBypass(void) {
  BK4819_SetAF(BK4819_AF_MUTE);
  BK4819_WriteRegister(BK4819_REG_7E, 0x302E);
}

void BK4819_PrepareTransmit(void) {
  BK4819_ExitBypass();
  BK4819_ExitTxMute();
  BK4819_TxOn_Beep();
}

void BK4819_TxOn_Beep(void) {
  BK4819_WriteRegister(BK4819_REG_37, 0x1D0F);
  BK4819_WriteRegister(BK4819_REG_52, 0x028F);
  BK4819_WriteRegister(BK4819_REG_30, 0x0000);
  BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
}

void BK4819_ExitSubAu(void) { BK4819_WriteRegister(BK4819_REG_51, 0x0000); }

void BK4819_EnableRX(void) {
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

void BK4819_EnableTXLink(void) {
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
  const uint16_t TAILS[] = {
      0x828F, 0xA28F, 0xC28F, 0xE28F, 0x046f,
  };
  BK4819_WriteRegister(BK4819_REG_52, TAILS[Tail]);
}

void BK4819_EnableCDCSS(void) {
  BK4819_GenTail(0); // CTC134
  BK4819_WriteRegister(BK4819_REG_51, 0x804A);
}

void BK4819_EnableCTCSS(void) {
  BK4819_GenTail(4); // CTC55
  BK4819_WriteRegister(BK4819_REG_51, 0x904A);
}

uint16_t BK4819_GetRSSI(void) {
  return BK4819_ReadRegister(BK4819_REG_67) & 0x1FF;
}

uint8_t BK4819_GetNoise(void) {
  return BK4819_ReadRegister(BK4819_REG_65) & 0x7F;
}

uint8_t BK4819_GetGlitch(void) {
  return BK4819_ReadRegister(BK4819_REG_63) & 0xFF;
}

uint8_t BK4819_GetSNR(void) { return (BK4819_ReadRegister(0x61) >> 8) & 0xFF; }

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

void BK4819_DisableFrequencyScan(void) {
  BK4819_WriteRegister(BK4819_REG_32, 0x0244);
}

void BK4819_EnableFrequencyScan(void) {
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

void BK4819_StopScan(void) {
  BK4819_DisableFrequencyScan();
  BK4819_Idle();
}

uint8_t BK4819_GetDTMF_5TONE_Code(void) {
  return (BK4819_ReadRegister(BK4819_REG_0B) >> 8) & 0x0F;
}

uint8_t BK4819_GetCDCSSCodeType(void) {
  return (BK4819_ReadRegister(BK4819_REG_0C) >> 14) & 3;
}

uint8_t BK4819_GetCTCType(void) {
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

void BK4819_PrepareFSKReceive(void) {
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

void BK4819_PlayRoger(void) {
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

void BK4819_PlayRogerMDC(void) {
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

void BK4819_Enable_AfDac_DiscMode_TxDsp(void) {
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
  UART_printf("BK4819_ToggleAFBit(%u)\n", on);
  UART_flush();
  uint16_t reg = BK4819_ReadRegister(BK4819_REG_47);
  reg &= ~(1 << 8);
  if (on)
    reg |= 1 << 8;
  BK4819_WriteRegister(BK4819_REG_47, reg);
}

void BK4819_ToggleAFDAC(bool on) {
  UART_printf("BK4819_ToggleAFDAC(%u)\n", on);
  UART_flush();
  uint16_t Reg = BK4819_ReadRegister(BK4819_REG_30);
  Reg &= ~BK4819_REG_30_ENABLE_AF_DAC;
  if (on)
    Reg |= BK4819_REG_30_ENABLE_AF_DAC;
  BK4819_WriteRegister(BK4819_REG_30, Reg);
}

void BK4819_TuneTo(uint32_t f) {
  UART_printf("BK4819_TuneTo(%u)\n", f);
  UART_flush();
  BK4819_SelectFilter(f);
  BK4819_SetFrequency(f);
  uint16_t reg = BK4819_ReadRegister(BK4819_REG_30);
  BK4819_WriteRegister(BK4819_REG_30, reg & ~BK4819_REG_30_ENABLE_VCO_CALIB);
  BK4819_WriteRegister(BK4819_REG_30, reg);
}

void BK4819_SetToneFrequency(uint16_t f) {
  BK4819_WriteRegister(BK4819_REG_71, (f * 103U) / 10U);
}

bool BK4819_IsSquelchOpen(void) {
  return (BK4819_ReadRegister(BK4819_REG_0C) >> 1) & 1;
}

void BK4819_ResetRSSI(void) {
  uint16_t Reg = BK4819_ReadRegister(BK4819_REG_30);
  Reg &= ~1;
  BK4819_WriteRegister(BK4819_REG_30, Reg);
  Reg |= 1;
  BK4819_WriteRegister(BK4819_REG_30, Reg);
}

void BK4819_SetGain(uint8_t gainIndex) {
  BK4819_WriteRegister(BK4819_REG_13, gainTable[gainIndex].regValue);
}

void BK4819_HandleInterrupts(void (*handler)(uint16_t intStatus)) {
  while (BK4819_ReadRegister(BK4819_REG_0C) & 1u) {
    BK4819_WriteRegister(BK4819_REG_02, 0);
    handler(BK4819_ReadRegister(BK4819_REG_02));
  }
}
