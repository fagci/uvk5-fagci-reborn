#include "vfos.h"
#include "../driver/eeprom.h"
#include "../driver/uart.h"

void CHS_Load(uint16_t num, CH *p) {
  EEPROM_ReadBuffer(SCANLISTS_OFFSET + num * SCANLIST_SIZE, p, SCANLIST_SIZE);
}

void CHS_Save(uint16_t num, CH *p) {
  EEPROM_WriteBuffer(SCANLISTS_OFFSET + num * SCANLIST_SIZE, p, SCANLIST_SIZE);
}
