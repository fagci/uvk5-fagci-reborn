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
 *
 */

#ifndef DRIVER_UART_H
#define DRIVER_UART_H

#include <stdint.h>

extern uint8_t UART_DMA_Buffer[256];
extern uint8_t gUartData[512];
extern uint8_t UART_IsLogEnabled;

void UART_Init();
void UART_Send(const void *pBuffer, uint32_t Size);
void UART_SendText(const void *str);
void UART_LogSend(const void *pBuffer, uint32_t Size);
void UART_LogSendText(const void *str);
void UART_printf(const char *str, ...);
uint16_t UART_HasData();
void UART_ResetData();
void UART_logf(uint8_t level, const char *pattern, ...);
void Log(const char *pattern, ...);
void UART_ToggleLog(bool on);
void UART_flush();

#endif
