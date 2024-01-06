#include "../driver/eeprom.h"
#include "../driver/gpio.h"
#include "../driver/i2c.h"
#include "../driver/system.h"
#include "../driver/uart.h"
#include "../inc/dp32g030/gpio.h"

bool gEepromWrite = false;
bool gEepromRead = false;

void EEPROM_ReadBuffer(uint16_t address, void *pBuffer, uint8_t size) {
  if (size == 0 || (address + size) > 0x2000) {
    return;
  }

  I2C_Start();

  I2C_Write(0xA0);

  I2C_Write((address >> 8) & 0xFF);
  I2C_Write((address >> 0) & 0xFF);

  I2C_Start();

  I2C_Write(0xA1);

  I2C_ReadBuffer(pBuffer, size);

  I2C_Stop();
  gEepromRead = true;
}

void EEPROM_WriteBuffer(uint16_t address, const void *pBuffer, uint8_t size) {
  for (uint8_t offset = 0; offset < size; offset += 8) {
    uint8_t rest = (size - offset);
    uint8_t n = rest > 8 ? 8 : rest;
    I2C_Start();

    I2C_Write(0xA0);

    I2C_Write(((address + offset) >> 8) & 0xFF);
    I2C_Write(((address + offset) >> 0) & 0xFF);

    I2C_WriteBuffer(pBuffer + offset, n);

    I2C_Stop();

    SYSTEM_DelayMs(10);
  }
  gEepromWrite = true;
}
