#include "battery.h"
#include "../board.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../settings.h"

uint16_t gBatteryCurrentVoltage;
uint16_t gBatteryVoltage;
uint8_t gBatteryDisplayLevel;
bool gChargingWithTypeC;
uint16_t gBatteryCurrent = 0;
uint8_t gBatteryPercent = 0;

const char *BATTERY_TYPE_NAMES[2] = {"1600mAh", "2200mAh"};
const char *BATTERY_STYLE_NAMES[3] = {"Plain", "Percent", "Voltage"};

const uint16_t Voltage2PercentageTable[][7][2] = {
    [BAT_1600] =
        {
            {828, 100},
            {814, 97},
            {760, 25},
            {729, 6},
            {630, 0},
            {0, 0},
            {0, 0},
        },

    [BAT_2200] =
        {
            {832, 100},
            {813, 95},
            {740, 60},
            {707, 21},
            {682, 5},
            {630, 0},
            {0, 0},
        },
};

uint8_t BATTERY_VoltsToPercent(const unsigned int voltage_10mV) {
  const uint16_t(*crv)[2] = Voltage2PercentageTable[gSettings.batteryType];
  const int mulipl = 1000;
  for (uint8_t i = 1; i < ARRAY_SIZE(Voltage2PercentageTable[BAT_2200]); i++) {
    if (voltage_10mV > crv[i][0]) {
      const int a =
          (crv[i - 1][1] - crv[i][1]) * mulipl / (crv[i - 1][0] - crv[i][0]);
      const int b = crv[i][1] - a * crv[i][0] / mulipl;
      const int p = a * voltage_10mV / mulipl + b;
      return p < 100 ? p : 100;
    }
  }

  return 0;
}

// calibrated
// const uint16_t BATTERY_CALIBRATION[6] = {1307, 1806, 1904, 1957, 2023, 2300};

// custom (mid from calibrated & max)
// const uint16_t BATTERY_CALIBRATION[6] = {1345, 1810, 1930, 2010, 2090, 2300};

// uint16_t voltages[4];

void BATTERY_UpdateBatteryInfo() {
  BOARD_ADC_GetBatteryInfo(&gBatteryCurrentVoltage, &gBatteryCurrent);
  BATTERY_GetReadings(false);
}

void BATTERY_GetReadings(bool bDisplayBatteryLevel) {
  gBatteryDisplayLevel = 0;

  gBatteryVoltage =
      (gBatteryCurrentVoltage * 760) / gSettings.batteryCalibration;
  gChargingWithTypeC = gBatteryCurrent >= 501;
  gBatteryPercent = BATTERY_VoltsToPercent(gBatteryVoltage);

  gBatteryDisplayLevel = gBatteryPercent / 10;
}
