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

#include "audio.h"
#include "board.h"
#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/syscon.h"
#include "driver/backlight.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "driver/gpio.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "driver/systick.h"
#include "helper/battery.h"
#include "misc.h"
#include "scheduler.h"
#include "settings.h"
#include <string.h>

const uint8_t SQ_UHF[10][6] = {
    {0x0a, 0x05, 0x5a, 0x64, 0x5a, 0x64}, {0x2c, 0x26, 0x35, 0x38, 0x20, 0x1e},
    {0x34, 0x2e, 0x30, 0x34, 0x18, 0x15}, {0x3a, 0x36, 0x2c, 0x2f, 0x14, 0x11},
    {0x42, 0x3e, 0x28, 0x2b, 0x11, 0x0e}, {0x48, 0x44, 0x24, 0x27, 0x0e, 0x0b},
    {0x50, 0x4c, 0x20, 0x23, 0x0b, 0x08}, {0x58, 0x54, 0x1c, 0x1f, 0x08, 0x05},
    {0x5e, 0x5c, 0x18, 0x1b, 0x03, 0x05}, {0x66, 0x64, 0x14, 0x17, 0x02, 0x04},
};

const uint8_t SQ_VHF[10][6] = {
    {0x32, 0x28, 0x41, 0x46, 0x5a, 0x64}, {0x4d, 0x46, 0x3a, 0x41, 0x20, 0x3c},
    {0x52, 0x4c, 0x34, 0x39, 0x17, 0x2d}, {0x58, 0x52, 0x2e, 0x33, 0x12, 0x1e},
    {0x5e, 0x58, 0x29, 0x2d, 0x0f, 0x14}, {0x64, 0x5e, 0x25, 0x29, 0x0a, 0x0f},
    {0x6a, 0x66, 0x21, 0x25, 0x09, 0x0d}, {0x70, 0x6c, 0x1c, 0x20, 0x08, 0x0c},
    {0x76, 0x72, 0x18, 0x1c, 0x07, 0x0b}, {0x7b, 0x78, 0x16, 0x19, 0x04, 0x08},
};

void RADIO_SetupRegisters() {
  uint32_t Frequency = 43400000;

  GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, false);
  BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);
  BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);

  BK4819_SetFilterBandwidth(BK4819_FILTER_BW_WIDE);

  BK4819_SetupPowerAmplifier(0, 0);

  while (BK4819_ReadRegister(BK4819_REG_0C) & 1U) {
    BK4819_WriteRegister(BK4819_REG_02, 0);
    SYSTEM_DelayMs(1);
  }
  BK4819_WriteRegister(BK4819_REG_3F, 0);
  BK4819_WriteRegister(BK4819_REG_7D, gEeprom.MIC_SENSITIVITY_TUNING | 0xE94F);
  uint8_t sql = 1;
  BK4819_SetupSquelch(SQ_UHF[sql][0], SQ_UHF[sql][1], SQ_UHF[sql][2],
                      SQ_UHF[sql][3], SQ_UHF[sql][4], SQ_UHF[sql][5]);
  BK4819_SetFrequency(Frequency);
  BK4819_SelectFilter(Frequency);
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, true);
  BK4819_WriteRegister(BK4819_REG_48, 0xB3A8);

  BK4819_DisableScramble();

  BK4819_DisableVox();
  BK4819_DisableDTMF();

  BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_SQUELCH_FOUND |
                                          BK4819_REG_3F_SQUELCH_LOST);
  BK4819_WriteRegister(0x40, (BK4819_ReadRegister(0x40) & ~(0b11111111111)) |
                                 0b10110101010);
  BK4819_SetAGC(0);
}

uint8_t v1 = 0;
uint8_t v2 = 0;
uint16_t rssi = 0;

#include "helper/measurements.h"
void Render() {
  memset(gStatusLine, 0, sizeof(gStatusLine));
  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
  memset(gFrameBuffer[1], v1, 64);
  memset(gFrameBuffer[1] + 64, v2, 64);
  memset(gFrameBuffer[3], 3, Rssi2PX(rssi, 0, 128));
  ST7565_BlitFullScreen();
}

bool rxState = false;

void ToggleRX(bool on) {
  if (on == rxState) {
    return;
  }
  rxState = on;
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, on);
  if (on) {
    GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
    BK4819_SetAF(BK4819_AF_OPEN);
    // BK4819_RX_TurnOn();
  } else {
    GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
    BK4819_SetAF(BK4819_AF_MUTE);
  }
}

void CheckSquelch() {
  bool rx = false;
  v2++;
  rssi = BK4819_GetRSSI();
  while (BK4819_ReadRegister(BK4819_REG_0C) & 1U) {
    BK4819_WriteRegister(BK4819_REG_02, 0);
    uint16_t Mask = BK4819_ReadRegister(BK4819_REG_02);
    if (Mask & BK4819_REG_02_SQUELCH_LOST) {
      rx = true;
    }

    if (Mask & BK4819_REG_02_SQUELCH_FOUND) {
      rx = false;
    }
    ToggleRX(rx);
  }
}

void Main(void) {
  // Enable clock gating of blocks we need.
  SYSCON_DEV_CLK_GATE = 0 | SYSCON_DEV_CLK_GATE_GPIOA_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_GPIOB_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_GPIOC_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_UART1_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_SPI0_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_SARADC_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_CRC_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_AES_BITS_ENABLE;

  SYSTICK_Init();
  BOARD_Init();

  // Not implementing authentic device checks

  BK4819_Init();
  BOARD_ADC_GetBatteryInfo(&gBatteryCurrentVoltage, &gBatteryCurrent);

  RADIO_SetupRegisters(true);

  for (uint8_t i = 0; i < 4; i++) {
    BOARD_ADC_GetBatteryInfo(&gBatteryVoltages[i], &gBatteryCurrent);
  }

  BATTERY_GetReadings(false);
  GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);

  TaskAdd(Render, 100, true);
  TaskAdd(CheckSquelch, 100, true);
  BK4819_TuneTo(43400000, true);

  while (1) {
    v1++;
    SYSTEM_DelayMs(100);
    // CheckSquelch();
  }
}
