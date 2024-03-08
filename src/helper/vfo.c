#include "vfo.h"
#include "../driver/eeprom.h"
#include "../settings.h"

void VFO_Load(uint16_t num, CH *p) {
  EEPROM_ReadBuffer(VFO_OFFSET + num * CH_SIZE, p, CH_SIZE);
}

void VFO_Save(uint16_t num, CH *p) {
  EEPROM_WriteBuffer(VFO_OFFSET + num * CH_SIZE, p, CH_SIZE);
}
