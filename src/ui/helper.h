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
