#include "../driver/eeprom.h"
#include "../driver/i2c.h"
#include "../driver/system.h"
#include "../external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"
#include <stddef.h>
#include <string.h>

bool gEepromWrite = false;
bool gEepromRead = false;

void EEPROM_ReadBuffer(uint32_t address, void *pBuffer, uint8_t size) {

  uint8_t IIC_ADD = 0xA0 | ((address / 0x10000) << 1);

  __disable_irq();
  I2C_Start();

  I2C_Write(IIC_ADD);
  I2C_Write(address >> 8);
  I2C_Write(address & 0xFF);

  I2C_Start();

  I2C_Write(IIC_ADD + 1);

  I2C_ReadBuffer(pBuffer, size);

  I2C_Stop();
  __enable_irq();

  gEepromRead = true;
}

void EEPROM_WriteBuffer(uint32_t address, const void *pBuffer, uint8_t size) {
  if (pBuffer == NULL) {
    return;
  }
  uint8_t buffer[128];
  EEPROM_ReadBuffer(address, buffer, size);
  if (memcmp(pBuffer, buffer, size) != 0) {

    const uint8_t *buf = (const uint8_t *)pBuffer;
    while (size) {
      uint16_t pageNum = address / 32;
      uint8_t rest = (pageNum + 1) * 32 - address;
      uint8_t n = size < rest ? size : rest;
      uint8_t IIC_ADD = 0xA0 | ((address / 0x10000) << 1);
      I2C_Start();
      I2C_Write(IIC_ADD);
      I2C_Write(address >> 8);
      I2C_Write(address & 0xFF);

      I2C_WriteBuffer(buf, n);

      I2C_Stop();
      SYSTEM_DelayMs(5);

      buf += n;
      address += n;
      size -= n;
    }
    gEepromWrite = true;
  }
}
