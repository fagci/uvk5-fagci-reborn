#include "settings.h"
#include "driver/eeprom.h"
#include "scheduler.h"

Settings gSettings;

uint8_t BL_TIME_VALUES[7] = {0, 5, 10, 20, 60, 120, 255};
const char *BL_TIME_NAMES[7] = {"Off",  "5s",   "10s", "20s",
                                "1min", "2min", "On"};

const char *BL_SQL_MODE_NAMES[3] = {"Off", "On", "Open"};

void SETTINGS_Save() {
  EEPROM_WriteBuffer(SETTINGS_OFFSET, &gSettings, SETTINGS_SIZE);
}

void SETTINGS_Load() {
  EEPROM_ReadBuffer(SETTINGS_OFFSET, &gSettings, SETTINGS_SIZE);
}

void SETTINGS_DelayedSave() {
  TaskRemove(SETTINGS_Save);
  TaskAdd("Settings save", SETTINGS_Save, 5000, false);
}
