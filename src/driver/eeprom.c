#include "../driver/eeprom.h"
#include "../driver/i2c.h"
#include "../driver/system.h"
#include "../settings.h"
#include "driver/uart.h"
#include "ARMCM0.h"
#include <stddef.h>
#include <string.h>

const uint32_t EEPROM_SIZES[8] = { 8192, 8192, 16384, 32768, 65536, 131072, 262144, 524288 };

const uint16_t PAGE_SIZES[8] = { 32, 32, 64, 64, 128, 128, 128, 128 };

uint32_t EEPROM_GetSize() {
  return EEPROM_SIZES[(int)__EEPROM_CODE];
};

uint16_t EEPROM_GetPageSize() {
  return PAGE_SIZES[(int)__EEPROM_CODE];
};

void EEPROM_ReadBuffer(uint32_t address, void *pBuffer, uint16_t size) {
    // uint8_t IIC_ADD = 0xA0 | address >> 15 & 14;
    uint8_t IIC_ADD = (uint8_t)(0xA0 | ((address / 0x10000) << 1));

    __disable_irq();
    I2C_Start();
    I2C_Write(IIC_ADD);
    I2C_Write((address >> 8) & 0xFF);
    I2C_Write(address & 0xFF);
    I2C_Start();
    I2C_Write(IIC_ADD + 1);
    I2C_ReadBuffer(pBuffer, size);
    I2C_Stop();
    __enable_irq();
}

void EEPROM_WriteBuffer(uint32_t address, void *pBuffer, uint16_t size) {
  if (pBuffer == NULL) {
    return;
  }
  
  uint8_t tmpBuffer[EEPROM_GetPageSize()];
  while (size) {
    uint16_t i = address % EEPROM_GetPageSize();
    uint16_t rest = EEPROM_GetPageSize() - i;
    uint16_t n = size < rest ? size : rest;

    EEPROM_ReadBuffer(address, tmpBuffer, n);
    if (memcmp(pBuffer, tmpBuffer, n) != 0) {
      // uint8_t IIC_ADD = 0xA0 | address >> 15 & 14;
      uint8_t IIC_ADD = (uint8_t)(0xA0 | ((address / 0x10000) << 1));

      I2C_Start();
      I2C_Write(IIC_ADD);
      I2C_Write((address >> 8) & 0xFF);
      I2C_Write(address & 0xFF);

      I2C_WriteBuffer(pBuffer, n);

      I2C_Stop();
      SYSTEM_DelayMs(10);
    }

    pBuffer += n;
    address += n;
    size -= n;
  }
}
