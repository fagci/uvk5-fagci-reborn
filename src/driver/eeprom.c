#include "../driver/eeprom.h"
#include "../driver/i2c.h"
#include "../driver/system.h"
#include "../driver/uart.h"
#include "../helper/measurements.h"

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
  const uint8_t CHUNK_SZ = 1;
  const uint8_t *buf = (const uint8_t *)pBuffer;
  while (size > 0) {
    uint8_t n = size > CHUNK_SZ ? CHUNK_SZ : size;

    I2C_Start();

    I2C_Write(0xA0);
    I2C_Write((address >> 8) & 0xFF);
    I2C_Write((address >> 0) & 0xFF);

    I2C_WriteBuffer(buf, n);

    I2C_Stop();

    SYSTEM_DelayMs(8);
    buf += n;
    address += n;
    size -= n;
  }
  gEepromWrite = true;
}
