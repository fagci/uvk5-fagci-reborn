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
#include "gpio.h"
#include "spi.h"
#include "system.h"
#include <stdint.h>

#define NEED_WAIT_FIFO                                                         \
  ((SPI0->FIFOST & SPI_FIFOST_TFF_MASK) != SPI_FIFOST_TFF_BITS_NOT_FULL)

uint8_t gStatusLine[LCD_WIDTH];
uint8_t gFrameBuffer[7][LCD_WIDTH];

bool gRedrawStatus = true;
bool gRedrawScreen = true;

void ST7565_DrawLine(uint8_t Column, uint8_t Line, uint16_t Size,
                     const uint8_t *pBitmap, bool bIsClearMode) {
  uint16_t i;

  SPI_ToggleMasterMode(&SPI0->CR, false);
  ST7565_SelectColumnAndLine(Column + 4U, Line);
  GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);

  if (bIsClearMode) {
    for (i = 0; i < Size; i++) {
      while (NEED_WAIT_FIFO)
        continue;
      SPI0->WDR = 0;
    }
  } else {
    for (i = 0; i < Size; i++) {
      while (NEED_WAIT_FIFO)
        continue;
      SPI0->WDR = pBitmap[i];
    }
  }

  SPI_WaitForUndocumentedTxFifoStatusBit();
  SPI_ToggleMasterMode(&SPI0->CR, true);
}

void ST7565_BlitFullScreen(void) {
  uint8_t Line;
  uint8_t Column;

  SPI_ToggleMasterMode(&SPI0->CR, false);
  ST7565_WriteByte(0x40);

  for (Line = 0; Line < ARRAY_SIZE(gFrameBuffer); Line++) {
    ST7565_SelectColumnAndLine(4U, Line + 1U);
    GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);
    for (Column = 0; Column < ARRAY_SIZE(gFrameBuffer[0]); Column++) {
      while (NEED_WAIT_FIFO)
        continue;
      SPI0->WDR = gFrameBuffer[Line][Column];
    }
    SPI_WaitForUndocumentedTxFifoStatusBit();
  }

  // SYSTEM_DelayMs(20);
  SPI_ToggleMasterMode(&SPI0->CR, true);
}

void ST7565_BlitStatusLine(void) {
  uint8_t i;

  SPI_ToggleMasterMode(&SPI0->CR, false);
  ST7565_WriteByte(0x40);
  ST7565_SelectColumnAndLine(4, 0);
  GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);

  for (i = 0; i < ARRAY_SIZE(gStatusLine); i++) {
    while (NEED_WAIT_FIFO)
      continue;
    SPI0->WDR = gStatusLine[i];
  }
  SPI_WaitForUndocumentedTxFifoStatusBit();
  SPI_ToggleMasterMode(&SPI0->CR, true);
}

void ST7565_FillScreen(uint8_t Value) {
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

void ST7565_Init(void) {
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
  ST7565_WriteByte(0x1F); // contrast
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

void ST7565_Configure_GPIO_B11(void) {
  GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_RES);
  SYSTEM_DelayMs(1);
  GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_ST7565_RES);
  SYSTEM_DelayMs(20);
  GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_ST7565_RES);
  SYSTEM_DelayMs(120);
}

void ST7565_SelectColumnAndLine(uint8_t Column, uint8_t Line) {
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

void ST7565_WriteByte(uint8_t Value) {
  GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_ST7565_A0);
  while (NEED_WAIT_FIFO)
    continue;
  SPI0->WDR = Value;
}

void ST7565_Render() {
  if (gRedrawStatus) {
    ST7565_BlitStatusLine();
    gRedrawStatus = false;
  }

  if (gRedrawScreen) {
    ST7565_BlitFullScreen();
    gRedrawScreen = false;
  }
}
