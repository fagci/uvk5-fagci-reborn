#include "vfo.h"
#include "../driver/eeprom.h"
#include "../settings.h"
#include "appsregistry.h"


void VFO_Load(uint16_t num, CH *p) {
  EEPROM_ReadBuffer(CHANNELS_END_OFFSET + num * CH_SIZE, p, CH_SIZE);
}

void VFO_Save(uint16_t num, CH *p) {
  EEPROM_WriteBuffer(CHANNELS_END_OFFSET + num * CH_SIZE, p, CH_SIZE);
}
