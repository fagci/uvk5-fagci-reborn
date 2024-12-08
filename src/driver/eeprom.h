#ifndef DRIVER_EEPROM_H
#define DRIVER_EEPROM_H

#include <stdbool.h>
#include <stdint.h>

extern bool gEepromWrite;

void EEPROM_ReadBuffer(uint32_t Address, void *pBuffer, uint16_t Size);
void EEPROM_WriteBuffer(uint32_t Address, void *pBuffer, uint16_t Size);
void EEPROM_ClearPage(uint16_t page);

#endif
