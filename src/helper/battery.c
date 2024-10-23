#include "battery.h"
#include "../board.h"
#include "../misc.h"
#include "../settings.h"

uint16_t gBatteryVoltage = 0;
uint16_t gBatteryCurrent = 0;
uint8_t gBatteryPercent = 0;
bool gChargingWithTypeC = true;

static uint16_t batAdcV = 0;
static uint16_t batAvgV = 0;

const char *BATTERY_TYPE_NAMES[3] = {"1600mAh", "2200mAh", "3500mAh"};
const char *BATTERY_STYLE_NAMES[3] = {"Icon", "%", "V"};

const uint16_t Voltage2PercentageTable[][11][2] = {
    [BAT_1600] =
        {
            {840, 100},
            {780, 90},
            {760, 80},
            {740, 70},
            {720, 60},
            {710, 50},
            {700, 40},
            {690, 30},
            {680, 20},
            {672, 10},
            {600, 0},
        },

    [BAT_2200] =
        {
            {840, 100},
            {800, 90},
            {784, 80},
            {768, 70},
            {756, 60},
            {748, 50},
            {742, 40},
            {738, 30},
            {732, 20},
            {720, 10},
            {600, 0},
        },
    [BAT_3500] =
        {
            {840, 100},
            {762, 90},
            {744, 80},
            {726, 70},
            {710, 60},
            {690, 50},
            {674, 40},
            {660, 30},
            {648, 20},
            {628, 10},
            {600, 0},
        },
};

uint8_t BATTERY_VoltsToPercent(const unsigned int voltage_10mV) {
  const uint16_t(*crv)[2] = Voltage2PercentageTable[gSettings.batteryType];
  const int mulipl = 1000;
  for (uint8_t i = 1;
       i < ARRAY_SIZE(Voltage2PercentageTable[gSettings.batteryType]); i++) {
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

uint32_t BATTERY_GetPreciseVoltage(uint16_t cal) {
  return batAvgV * 76000 / cal;
}
