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

#include "../helper/channels.h"
#include <stdbool.h>
#include <stdint.h>

void UART_Init(void);
void UART_Send(const void *pBuffer, uint32_t Size);
void UART_printf(const char *str, ...);

bool UART_IsCommandAvailable(void);
void UART_HandleCommand(void);
void Log(const char *pattern, ...);
void LogUart(const char *const str);
void PrintCh(uint16_t chNum, CH *ch);

#endif
