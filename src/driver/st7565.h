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

#ifndef DRIVER_ST7565_H
#define DRIVER_ST7565_H

#include <stdint.h>

#define LCD_WIDTH 128
#define LCD_HEIGHT 64
#define LCD_XCENTER 64
#define LCD_YCENTER 32

extern bool gRedrawScreen;
extern uint8_t gFrameBuffer[8][LCD_WIDTH];

void ST7565_Blit();
void ST7565_Init();
void ST7565_WriteByte(uint8_t Value);
void ST7565_Render();

#endif
