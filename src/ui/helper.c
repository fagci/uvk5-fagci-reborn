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

#include "helper.h"
#include "../driver/st7565.h"
#include "../misc.h"
#include "../radio.h"
#include "components.h"
#include "inputbox.h"

void UI_ClearStatus() { memset(gFrameBuffer[0], 0, BATTERY_X - 1); }
void UI_ClearStatusFull() { memset(gFrameBuffer[0], 0, LCD_WIDTH); }

void UI_ClearScreen() { memset(gFrameBuffer, 0, sizeof(gFrameBuffer)); }

