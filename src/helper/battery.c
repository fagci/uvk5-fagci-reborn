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

uint16_t gBatteryCalibration[6]= {520, 689, 724, 760, 771, 2300};
uint16_t gBatteryCurrentVoltage;
uint16_t gBatteryCurrent;
uint16_t gBatteryVoltages[4];
uint16_t gBatteryVoltageAverage;

uint8_t gBatteryDisplayLevel;

bool gChargingWithTypeC;
bool gLowBattery;
bool gLowBatteryBlink;

volatile uint16_t gBatterySave;

uint16_t gBatteryCheckCounter;

void BATTERY_UpdateBatteryInfo() {
  BOARD_ADC_GetBatteryInfo(&gBatteryCurrentVoltage, &gBatteryCurrent);
  for (uint8_t i = 0; i < 4; i++) {
    BOARD_ADC_GetBatteryInfo(&gBatteryVoltages[i], &gBatteryCurrent);
  }
  BATTERY_GetReadings(false);
}

void BATTERY_GetReadings(bool bDisplayBatteryLevel) {
  uint16_t Voltage = Mid(gBatteryVoltages, ARRAY_SIZE(gBatteryVoltages));

  gBatteryDisplayLevel = 0;
  for (int8_t i = ARRAY_SIZE(gBatteryCalibration) - 1; i >= 0; --i) {
    if (Voltage > gBatteryCalibration[i]) {
      gBatteryDisplayLevel = i + 1;
      break;
    }
  }

  gBatteryVoltageAverage = (Voltage * 760) / gBatteryCalibration[3];
  gChargingWithTypeC = gBatteryCurrent >= 501;

  if (gBatteryDisplayLevel < 2) {
    gLowBattery = true;
  } else {
    gLowBattery = false;
  }
}
