#ifndef DRIVER_EEPROM_H
#define DRIVER_EEPROM_H

#include <stdint.h>

extern bool gEepromRead;
extern bool gEepromWrite;

void EEPROM_ReadBuffer(uint32_t Address, void *pBuffer, uint8_t Size);
void EEPROM_WriteBuffer(uint32_t Address, void *pBuffer, uint8_t Size);

#endif
