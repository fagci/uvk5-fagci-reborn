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
#if defined(ENABLE_AM_FIX)
#include "am_fix.h"
#endif
#include "board.h"
#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/syscon.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "driver/gpio.h"
#include "driver/system.h"
#include "driver/systick.h"
#include <string.h>
#if defined(ENABLE_UART)
#include "driver/uart.h"
#endif
#include "helper/battery.h"
#include "misc.h"
#include "settings.h"

void _putchar(char c) {
#if defined(ENABLE_UART)
  UART_Send((uint8_t *)&c, 1);
#endif
}

void RADIO_SetupRegisters(bool bSwitchToFunction0) {
  BK4819_FilterBandwidth_t Bandwidth;
  uint16_t Status;
  uint32_t Frequency = 43400000;

  GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, false);

  Bandwidth = BK4819_FILTER_BW_WIDE;
  BK4819_SetFilterBandwidth(Bandwidth);

  BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);
  BK4819_SetupPowerAmplifier(0, 0);
  BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);

  while (1) {
    Status = BK4819_ReadRegister(BK4819_REG_0C);
    if ((Status & 1U) == 0) { // INTERRUPT REQUEST
      break;
    }
    BK4819_WriteRegister(BK4819_REG_02, 0);
    SYSTEM_DelayMs(1);
  }
  BK4819_WriteRegister(BK4819_REG_3F, 0);
  BK4819_WriteRegister(BK4819_REG_7D, gEeprom.MIC_SENSITIVITY_TUNING | 0xE94F);
  // Frequency = gRxVfo->pRX->Frequency;
  /* BK4819_SetupSquelch(
      gRxVfo->SquelchOpenRSSIThresh, gRxVfo->SquelchCloseRSSIThresh,
      gRxVfo->SquelchOpenNoiseThresh, gRxVfo->SquelchCloseNoiseThresh,
      gRxVfo->SquelchCloseGlitchThresh, gRxVfo->SquelchOpenGlitchThresh); */
  BK4819_SetFrequency(Frequency);
  BK4819_SelectFilter(Frequency);
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, true);
  BK4819_WriteRegister(BK4819_REG_48, 0xB3A8);

  BK4819_DisableScramble();

  BK4819_DisableVox();
  BK4819_DisableDTMF();
  BK4819_WriteRegister(0x40, (BK4819_ReadRegister(0x40) & ~(0b11111111111)) |
                                 0b10110101010);
}

void Main(void) {
  uint8_t i;

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

#if defined(ENABLE_UART)
  UART_Init();
  UART_Send(UART_Version, sizeof(UART_Version));
#endif

  // Not implementing authentic device checks

  memset(&gEeprom, 0, sizeof(gEeprom));

  BK4819_Init();
  BOARD_ADC_GetBatteryInfo(&gBatteryCurrentVoltage, &gBatteryCurrent);
  BOARD_EEPROM_Init();
  BOARD_EEPROM_LoadCalibration();

  RADIO_SetupRegisters(true);

  for (i = 0; i < 4; i++) {
    BOARD_ADC_GetBatteryInfo(&gBatteryVoltages[i], &gBatteryCurrent);
  }

  BATTERY_GetReadings(false);
  BACKLIGHT_TurnOn();
  SYSTEM_DelayMs(1000);

#include "driver/st7565.h"
  while (1) {
    memset(gStatusLine, 0, sizeof(gStatusLine));
    memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
    memset(gFrameBuffer[1], 0b10101010, 128);
    ST7565_BlitFullScreen();
    SYSTEM_DelayMs(1000);
  }
}
