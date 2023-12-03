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

#ifndef UI_UI_H
#define UI_UI_H

#include "../external/printf/printf.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void UI_PrintString(const char *pString, uint8_t Start, uint8_t End,
                    uint8_t Line);
void UI_PrintStringSmall(const char *pString, uint8_t Start, uint8_t End,
                         uint8_t Line);
void UI_PrintStringSmallBold(const char *pString, uint8_t Start, uint8_t End,
                             uint8_t Line);
void UI_DisplayFrequency(const char *pDigits, uint8_t X, uint8_t Y,
                         bool bDisplayLeadingZero, bool bFlag);
void UI_DisplaySmallDigits(uint8_t Size, const char *pString, uint8_t X,
                           uint8_t Y);
void PutPixel(uint8_t x, uint8_t y, uint8_t fill);
void PutPixelStatus(uint8_t x, uint8_t y, bool fill);
void DrawHLine(int sy, int ey, int nx, bool fill);
void UI_PrintStringSmallest(const char *pString, uint8_t x, uint8_t y,
                            bool statusbar, bool fill);
/* void UI_ClearAppScreen();
void UI_DrawScanListFlag(uint8_t *pLine, uint8_t attrs); */
bool UI_NoChannelName(const char *channelName);
void UI_ClearStatus();
void UI_ClearScreen();

#define SHOW_ITEMS(value)                                                      \
  do {                                                                         \
    char items[ARRAY_SIZE(value)][16] = {0};                                   \
    for (uint8_t i = 0; i < ARRAY_SIZE(value); ++i) {                          \
      strncpy(items[i], value[i], 15);                                         \
    }                                                                          \
    UI_ShowItems(items, ARRAY_SIZE(value), subMenuIndex);                      \
  } while (0)

#endif
