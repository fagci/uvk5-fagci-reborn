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

#include "../inc/dp32g030/uart.h"
#include "../external/printf/printf.h"
#include "../inc/dp32g030/dma.h"
#include "../inc/dp32g030/syscon.h"
#include "../misc.h"
#include "../scheduler.h"
#include "system.h"
#include "uart.h"
#include <string.h>

uint8_t UART_IsLogEnabled = 0;
uint8_t UART_DMA_Buffer[256];

void UART_Init() {
  uint32_t Delta;
  uint32_t Positive;
  uint32_t Frequency;

  UART1->CTRL =
      (UART1->CTRL & ~UART_CTRL_UARTEN_MASK) | UART_CTRL_UARTEN_BITS_DISABLE;
  Delta = SYSCON_RC_FREQ_DELTA;
  Positive = (Delta & SYSCON_RC_FREQ_DELTA_RCHF_SIG_MASK) >>
             SYSCON_RC_FREQ_DELTA_RCHF_SIG_SHIFT;
  Frequency = (Delta & SYSCON_RC_FREQ_DELTA_RCHF_DELTA_MASK) >>
              SYSCON_RC_FREQ_DELTA_RCHF_DELTA_SHIFT;
  Frequency = Positive ? Frequency + CPU_CLOCK_HZ : CPU_CLOCK_HZ - Frequency;

  UART1->BAUD = Frequency / 39053U;
  UART1->CTRL = UART_CTRL_RXEN_BITS_ENABLE | UART_CTRL_TXEN_BITS_ENABLE |
                UART_CTRL_RXDMAEN_BITS_ENABLE;
  UART1->RXTO = 4;
  UART1->FC = 0;
  UART1->FIFO = UART_FIFO_RF_LEVEL_BITS_8_BYTE | UART_FIFO_RF_CLR_BITS_ENABLE |
                UART_FIFO_TF_CLR_BITS_ENABLE;
  UART1->IE = 0;

  DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_DISABLE;

  DMA_CH0->MSADDR = (uint32_t)(uintptr_t)&UART1->RDR;
  DMA_CH0->MDADDR = (uint32_t)(uintptr_t)UART_DMA_Buffer;
  DMA_CH0->MOD = 0
                 // Source
                 | DMA_CH_MOD_MS_ADDMOD_BITS_NONE |
                 DMA_CH_MOD_MS_SIZE_BITS_8BIT |
                 DMA_CH_MOD_MS_SEL_BITS_HSREQ_MS1
                 // Destination
                 | DMA_CH_MOD_MD_ADDMOD_BITS_INCREMENT |
                 DMA_CH_MOD_MD_SIZE_BITS_8BIT | DMA_CH_MOD_MD_SEL_BITS_SRAM;
  DMA_INTEN = 0;
  DMA_INTST =
      0 | DMA_INTST_CH0_TC_INTST_BITS_SET | DMA_INTST_CH1_TC_INTST_BITS_SET |
      DMA_INTST_CH2_TC_INTST_BITS_SET | DMA_INTST_CH3_TC_INTST_BITS_SET |
      DMA_INTST_CH0_THC_INTST_BITS_SET | DMA_INTST_CH1_THC_INTST_BITS_SET |
      DMA_INTST_CH2_THC_INTST_BITS_SET | DMA_INTST_CH3_THC_INTST_BITS_SET;
  DMA_CH0->CTR = 0 | DMA_CH_CTR_CH_EN_BITS_ENABLE |
                 ((0xFF << DMA_CH_CTR_LENGTH_SHIFT) & DMA_CH_CTR_LENGTH_MASK) |
                 DMA_CH_CTR_LOOP_BITS_ENABLE | DMA_CH_CTR_PRI_BITS_MEDIUM;
  UART1->IF = UART_IF_RXTO_BITS_SET;

  DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_ENABLE;

  UART1->CTRL |= UART_CTRL_UARTEN_BITS_ENABLE;
}

void UART_Send(const void *pBuffer, uint32_t Size) {
  const uint8_t *pData = (const uint8_t *)pBuffer;
  uint32_t i;

  for (i = 0; i < Size; i++) {
    UART1->TDR = pData[i];
    while ((UART1->IF & UART_IF_TXFIFO_FULL_MASK) !=
           UART_IF_TXFIFO_FULL_BITS_NOT_SET) {
    }
  }
}

void UART_SendText(const void *str) {
  if (str)
    UART_Send(str, strlen(str));
}

void UART_LogSend(const void *pBuffer, uint32_t Size) {
  if (UART_IsLogEnabled)
    UART_Send(pBuffer, Size);
}

void UART_LogSendText(const void *str) {
  if (UART_IsLogEnabled && str)
    UART_Send(str, strlen(str));
}

static char sendBuffer[512] = {0};
static uint32_t sendBufferIndex = 0;

void UART_flush() {
  UART_Send(sendBuffer, sendBufferIndex);
  sendBufferIndex = 0;
}

void UART_printf(const char *str, ...) {
  char text[128];
  int len;

  va_list va;
  va_start(va, str);
  len = vsnprintf(text, sizeof(text), str, va);
  va_end(va);

  memcpy(sendBuffer + sendBufferIndex, text, len);
  sendBufferIndex += len;

  if (sendBufferIndex >= 384) {
    UART_flush();
  }
}

void UART_ToggleLog(bool on) {
  if (on && UART_IsLogEnabled < 5) {
    UART_IsLogEnabled++;
  }
  if (!on && UART_IsLogEnabled > 0) {
    UART_IsLogEnabled--;
  }
}

void UART_logf(uint8_t level, const char *pattern, ...) {
  if (UART_IsLogEnabled >= level) {
    char text[128];
    va_list args;
    va_start(args, pattern);
    vsnprintf(text, sizeof(text), pattern, args);
    va_end(args);
    UART_printf("%u %s\n", elapsedMilliseconds, text);
  }
}

void Log(const char *pattern, ...) {
    char text[128];
    va_list args;
    va_start(args, pattern);
    vsnprintf(text, sizeof(text), pattern, args);
    va_end(args);
    UART_printf("%u %s\n", elapsedMilliseconds, text);
    UART_flush();
}

#define DMA_INDEX(x, y) (((x) + (y)) % sizeof(UART_DMA_Buffer))

static uint16_t write_index = 0;
uint8_t gUartData[512] = {0};
static uint16_t bufIndex = 0;
static uint8_t countdown = 0;

uint16_t UART_HasData() {
  uint16_t DmaLength = DMA_CH0->ST & 0xFFFU;

  if (write_index == DmaLength) {
    if (bufIndex && --countdown == 0) {
      bufIndex = 0;
      return true;
    }
    return false;
  }

  uint16_t n = 0;

  if (write_index > DmaLength) { // end of buffer
    n = 256 - write_index;
    memcpy(gUartData + bufIndex, UART_DMA_Buffer + write_index, n);
    memset(UART_DMA_Buffer + write_index, 0, n);
    bufIndex += n;

    n = write_index;
    memcpy(gUartData + bufIndex, UART_DMA_Buffer, n);
    memset(UART_DMA_Buffer, 0, n);
    bufIndex += n;

  } else {
    n = DmaLength - write_index;
    memcpy(gUartData + bufIndex, UART_DMA_Buffer + write_index, n);
    memset(UART_DMA_Buffer + write_index, 0, n);
    bufIndex += n;
  }

  countdown = 10;

  write_index = DmaLength;

  return false;
}
