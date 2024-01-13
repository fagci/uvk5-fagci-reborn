#include "battery.h"
#include "../board.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../settings.h"

uint16_t gBatteryVoltage = 0;
uint16_t gBatteryCurrent = 0;
uint8_t gBatteryPercent = 0;
bool gChargingWithTypeC = true;

static uint16_t batAdcV = 0;
static uint16_t batAvgV = 0;

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

void BATTERY_UpdateBatteryInfo() {
  BOARD_ADC_GetBatteryInfo(&batAdcV, &gBatteryCurrent);
  bool charg = gBatteryCurrent >= 501;
  if (batAvgV == 0 || charg != gChargingWithTypeC) {
    batAvgV = batAdcV;
  } else {
    batAvgV = batAvgV - (batAvgV - batAdcV) / 7;
  }

  gBatteryVoltage = (batAvgV * 760) / gSettings.batteryCalibration;
  gChargingWithTypeC = charg;
  gBatteryPercent = BATTERY_VoltsToPercent(gBatteryVoltage);
}
