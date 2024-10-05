#ifndef DRIVER_EEPROM_H
#define DRIVER_EEPROM_H

#include <stdbool.h>
#include <stdint.h>

void EEPROM_ReadBuffer(uint32_t address, void *pBuffer, uint16_t size);
void EEPROM_WriteBuffer(uint32_t address, void *pBuffer, uint16_t size);

// #if __EEPROM == BL24C128
#if __EEPROM == 2
  #define __EEPROM_SIZE (uint32_t)16384
  #define __EEPROM_PAGESIZE (uint8_t)64
  #define __EEPROM_CODE (uint8_t)0x02
// #elif __EEPROM == BL24C256
#elif __EEPROM == 3
  #define __EEPROM_SIZE (uint32_t)32768
  #define __EEPROM_PAGESIZE (uint8_t)64
  #define __EEPROM_CODE (uint8_t)0x03
// #elif __EEPROM == BL24C512
#elif __EEPROM == 4
  #define __EEPROM_SIZE (uint32_t)65536
  #define __EEPROM_PAGESIZE (uint8_t)128
  #define __EEPROM_CODE (uint8_t)0x04
// #elif __EEPROM == BL24C1024
#elif __EEPROM == 5
  #define __EEPROM_SIZE (uint32_t)131072
  #define __EEPROM_PAGESIZE (uint8_t)128
  #define __EEPROM_CODE (uint8_t)0x05
// #elif __EEPROM == M24M02
#elif __EEPROM == 6
  #define __EEPROM_SIZE (uint32_t)262144
  #define __EEPROM_PAGESIZE (uint8_t)128
  #define __EEPROM_CODE (uint8_t)0x06
// #elif __EEPROM == M24M02x2
#elif __EEPROM == 7
  #define __EEPROM_SIZE (uint32_t)524288
  #define __EEPROM_PAGESIZE (uint8_t)128
  #define __EEPROM_CODE (uint8_t)0x07
#else
  #define __EEPROM_SIZE (uint32_t)8192
  #define __EEPROM_PAGESIZE (uint8_t)32
  #define __EEPROM_CODE (uint8_t)0x01
#endif

#endif
