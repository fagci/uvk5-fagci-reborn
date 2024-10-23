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

#ifndef BATTERY_H
#define BATTERY_H

#include <stdbool.h>
#include <stdint.h>

#define BAT_WARN_PERCENT 15

extern uint16_t gBatteryVoltage;
extern uint16_t gBatteryCurrent;
extern uint8_t gBatteryPercent;
extern bool gChargingWithTypeC;

extern const char *BATTERY_TYPE_NAMES[3];
extern const char *BATTERY_STYLE_NAMES[3];

void BATTERY_UpdateBatteryInfo();
uint32_t BATTERY_GetPreciseVoltage(uint16_t cal);

#endif
