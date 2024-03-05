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

#include "st7565.h"
#include "../inc/dp32g030/gpio.h"
#include "../inc/dp32g030/spi.h"
#include "../misc.h"
#include "../settings.h"
#include "gpio.h"
#include "spi.h"
#include "system.h"
#include <stdint.h>

#define NEED_WAIT_FIFO                                                         \
  ((SPI0->FIFOST & SPI_FIFOST_TFF_MASK) != SPI_FIFOST_TFF_BITS_NOT_FULL)

uint8_t gFrameBuffer[8][LCD_WIDTH];

bool gRedrawScreen = true;

static void ST7565_Configure_GPIO_B11() {
  GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_RES);
  SYSTEM_DelayMs(1);
  GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_ST7565_RES);
  SYSTEM_DelayMs(20);
  GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_RES);
  SYSTEM_DelayMs(120);
}

static void ST7565_SelectColumnAndLine(uint8_t Column, uint8_t Line) {
  GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);
  while (NEED_WAIT_FIFO)
    continue;
  SPI0->WDR = Line + 0xB0;
  while (NEED_WAIT_FIFO)
    continue;
  SPI0->WDR = ((Column >> 4) & 0x0F) | 0x10;
  while (NEED_WAIT_FIFO)
    continue;
  SPI0->WDR = ((Column >> 0) & 0x0F);
  SPI_WaitForUndocumentedTxFifoStatusBit();
}

static void ST7565_FillScreen(uint8_t Value) {
  uint8_t i, j;

  SPI_ToggleMasterMode(&SPI0->CR, false);
  for (i = 0; i < 8; i++) {
    ST7565_SelectColumnAndLine(0, i);
    GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);
    for (j = 0; j < 132; j++) {
      while (NEED_WAIT_FIFO)
        continue;
      SPI0->WDR = Value;
    }
    SPI_WaitForUndocumentedTxFifoStatusBit();
  }
  SPI_ToggleMasterMode(&SPI0->CR, true);
}

static void fix() {
  SPI_ToggleMasterMode(&SPI0->CR, false);

  ST7565_WriteByte(0xA2); // bias 9

  ST7565_WriteByte(0xC0); // COM normal

  ST7565_WriteByte(0xA1); // reverse ADC

  ST7565_WriteByte(0xA6); // normal screen

  ST7565_WriteByte(0xA4); // all points normal

  ST7565_WriteByte(0x24); // ???

  ST7565_WriteByte(0x81);                    //
  ST7565_WriteByte(23 + gSettings.contrast); // brightness 0 ~ 63

  ST7565_WriteByte(0x40); // start line ?
  ST7565_WriteByte(0xAF); // display on ?

  SPI_WaitForUndocumentedTxFifoStatusBit();

  SPI_ToggleMasterMode(&SPI0->CR, true);
}

void ST7565_Blit() {
  uint8_t Line;
  uint8_t Column;

  fix();
  SPI_ToggleMasterMode(&SPI0->CR, false);
  ST7565_WriteByte(0x40);

  for (Line = 0; Line < ARRAY_SIZE(gFrameBuffer); Line++) {
    ST7565_SelectColumnAndLine(4U, Line);
    GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);
    for (Column = 0; Column < ARRAY_SIZE(gFrameBuffer[0]); Column++) {
      while (NEED_WAIT_FIFO)
        continue;
      SPI0->WDR = gFrameBuffer[Line][Column];
    }
    SPI_WaitForUndocumentedTxFifoStatusBit();
  }

  SPI_ToggleMasterMode(&SPI0->CR, true);
}

void ST7565_Init() {
  SPI0_Init();
  ST7565_Configure_GPIO_B11();
  SPI_ToggleMasterMode(&SPI0->CR, false);
  ST7565_WriteByte(0xE2);
  SYSTEM_DelayMs(0x78);
  ST7565_WriteByte(0xA2);
  ST7565_WriteByte(0xC0);
  ST7565_WriteByte(0xA1);
  ST7565_WriteByte(0xA6);
  ST7565_WriteByte(0xA4);
  ST7565_WriteByte(0x24);
  ST7565_WriteByte(0x81);
  // ST7565_WriteByte(0x1F); // contrast
  ST7565_WriteByte(23 + gSettings.contrast); // brightness 0 ~ 63
  ST7565_WriteByte(0x2B);
  SYSTEM_DelayMs(1);
  ST7565_WriteByte(0x2E);
  SYSTEM_DelayMs(1);
  ST7565_WriteByte(0x2F);
  ST7565_WriteByte(0x2F);
  ST7565_WriteByte(0x2F);
  ST7565_WriteByte(0x2F);
  SYSTEM_DelayMs(0x28);
  ST7565_WriteByte(0x40);
  ST7565_WriteByte(0xAF);
  SPI_WaitForUndocumentedTxFifoStatusBit();
  SPI_ToggleMasterMode(&SPI0->CR, true);
  ST7565_FillScreen(0x00);
}

void ST7565_WriteByte(uint8_t Value) {
  GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);
  while (NEED_WAIT_FIFO)
    continue;
  SPI0->WDR = Value;
}

void ST7565_Render() {
  if (gRedrawScreen) {
    ST7565_Blit();
    gRedrawScreen = false;
  }
}
