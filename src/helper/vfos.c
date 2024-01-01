#include "vfos.h"
#include "../driver/eeprom.h"

const uint32_t VFOS_OFFSET = 8196 - VFO_SIZE * 2;

void VFOS_Load(uint16_t num, VFO *p) {
  EEPROM_ReadBuffer(VFO_SIZE - num * VFO_SIZE, p, VFO_SIZE);
}

void VFOS_Save(uint16_t num, VFO *p) {
  EEPROM_WriteBuffer(VFO_SIZE - num * VFO_SIZE, p, VFO_SIZE);
}
