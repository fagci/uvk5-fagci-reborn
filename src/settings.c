#include "settings.h"
#include "driver/eeprom.h"

Settings gSettings;

void SETTINGS_Save() {
  EEPROM_WriteBuffer(SETTINGS_OFFSET, &gSettings, SETTINGS_SIZE);
}

void SETTINGS_Load() {
  EEPROM_ReadBuffer(SETTINGS_OFFSET, &gSettings, SETTINGS_SIZE);
}
