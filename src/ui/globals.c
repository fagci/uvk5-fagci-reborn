#include "globals.h"

const char *ON_OFF_NAMES[] = {"Off", "On"};
const char *YES_NO_NAMES[] = {"No", "Yes"};
const char *UPCONVERTER_NAMES[3] = {"None", "50M", "125M"};
const char *DEVIATION_NAMES[] = {"", "+", "-"};
const char *DCS_NAMES[] = {"", "CT", "DCS", "DCS"};
const char *SQ_TYPE_NAMES[4] = {"RNG", "RG", "RN", "R"};
const char *POWER_NAMES[] = {"LOW", "MID", "HIGH"};
const char *BW_NAMES[3] = {"25k", "12.5k", "6.25k"};
const char *FILTER_BOUND_NAMES[] = {"240MHz", "280MHz"};
const char *BL_SQL_MODE_NAMES[3] = {"Off", "On", "Open"};
const char *TX_POWER_NAMES[3] = {"Low", "Mid", "High"};
const char *TX_OFFSET_NAMES[3] = {"Unset", "+", "-"};
const char *BATTERY_TYPE_NAMES[3] = {"1600mAh", "2200mAh", "3500mAh"};
const char *BATTERY_STYLE_NAMES[3] = {"Plain", "Percent", "Voltage"};
const char *modulationTypeOptions[6] = {"FM", "AM", "SSB", "BYP", "RAW", "WFM"};

const char *TX_STATE_NAMES[7] = {"TX Off",         "TX On",    "VOL HIGH",
                                 "BAT LOW",        "DISABLED", "UPCONVERTER",
                                 "POWER OVERDRIVE"};

const char *BL_TIME_NAMES[7] = {"Off",  "5s",   "10s", "20s",
                                "1min", "2min", "On"};

const char *TX_ALLOW_NAMES[5] = {"Disallow", "LPD+PMR", "LPD+PMR+SAT", "HAM",
                                 "Allow"};

const char *EEPROM_TYPE_NAMES[8] = {
    "Undefined 1",       // 000
    "Undefined 2",       // 001
    "BL24C64 (default)", // 010
    "BL24C128",          // 011
    "BL24C256",          // 100
    "BL24C512",          // 101
    "BL24C1024",         // 110
    "M24M02 (x1)",       // 111
};
