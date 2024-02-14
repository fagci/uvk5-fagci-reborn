#include "../driver/eeprom.h"
#include "../driver/i2c.h"
#include "../driver/system.h"
#include "../external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"

bool gEepromWrite = false;
bool gEepromRead = false;

void EEPROM_ReadBuffer(uint16_t address, void *pBuffer, uint8_t size) {
  __disable_irq();
  I2C_Start();

  I2C_Write(0xA0);
  I2C_Write(address >> 8);
  I2C_Write(address & 0xFF);

  I2C_Start();

  I2C_Write(0xA1);

  I2C_ReadBuffer(pBuffer, size);

  I2C_Stop();
  __enable_irq();

  gEepromRead = true;
}

void EEPROM_WriteBuffer(uint16_t address, const void *pBuffer, uint8_t size) {
  // TODO: write until new page starts
  const uint8_t *buf = (const uint8_t *)pBuffer;
  while (size) {
    uint16_t pageNum = address / 32;
    uint8_t rest = (pageNum + 1) * 32 - address;
    uint8_t n = size < rest ? size : rest;
    I2C_Start();

    I2C_Write(0xA0);
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
