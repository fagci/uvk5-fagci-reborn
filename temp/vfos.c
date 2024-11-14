#include "vfos.h"
#include "../driver/eeprom.h"

void VFOS_Load(uint16_t num, VFO *p) {
  EEPROM_ReadBuffer(VFOS_OFFSET + num * CH_SIZE, p, CH_SIZE);
}

void VFOS_Save(uint16_t num, VFO *p) {
  EEPROM_WriteBuffer(VFOS_OFFSET + num * CH_SIZE, p, CH_SIZE);
}
