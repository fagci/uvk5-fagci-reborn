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

#include "battery.h"
#include "../board.h"
#include "../helper/measurements.h"
#include "../misc.h"

uint16_t gBatteryCurrentVoltage;
uint16_t gBatteryVoltage;
uint8_t gBatteryDisplayLevel;
bool gChargingWithTypeC;
uint16_t gBatteryCurrent = 0;

// calibrated
// const uint16_t BATTERY_CALIBRATION[6] = {1307, 1806, 1904, 1957, 2023, 2300};

// custom (mid from calibrated & max)
const uint16_t BATTERY_CALIBRATION[6] = {1345, 1810, 1930, 2010, 2090, 2300};

uint16_t voltages[4];

void BATTERY_UpdateBatteryInfo() {
  BOARD_ADC_GetBatteryInfo(&gBatteryCurrentVoltage, &gBatteryCurrent);
  for (uint8_t i = 0; i < 4; i++) {
    BOARD_ADC_GetBatteryInfo(&voltages[i], &gBatteryCurrent);
  }
  BATTERY_GetReadings(false);
}

void BATTERY_GetReadings(bool bDisplayBatteryLevel) {
  uint16_t Voltage = Mid(voltages, ARRAY_SIZE(voltages));

  gBatteryDisplayLevel = 0;
  for (int8_t i = ARRAY_SIZE(BATTERY_CALIBRATION) - 1; i >= 0; --i) {
    if (Voltage > BATTERY_CALIBRATION[i]) {
      gBatteryDisplayLevel = i + 1;
      break;
    }
  }

  gBatteryVoltage = (Voltage * 760) / BATTERY_CALIBRATION[3];
  gChargingWithTypeC = gBatteryCurrent >= 501;
}
