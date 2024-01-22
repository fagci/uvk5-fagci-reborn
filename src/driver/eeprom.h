#ifndef DRIVER_EEPROM_H
#define DRIVER_EEPROM_H

#include <stdbool.h>
#include <stdint.h>

#define EEPROM_SIZE 8192
// #define EEPROM_SIZE 32768

extern bool gEepromRead;
extern bool gEepromWrite;

void EEPROM_ReadBuffer(uint16_t Address, void *pBuffer, uint8_t Size);
void EEPROM_WriteBuffer(uint16_t Address, const void *pBuffer, uint8_t Size);

#endif
