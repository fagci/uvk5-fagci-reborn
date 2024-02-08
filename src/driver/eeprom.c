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
  const uint8_t *buf = (const uint8_t *)pBuffer;
  while (size--) {
    I2C_Start();

    I2C_Write(0xA0);
    I2C_Write(address >> 8);
    I2C_Write(address & 0xFF);

    I2C_WriteBuffer(buf, 1);

    I2C_Stop();
    SYSTEM_DelayMs(5);

    buf++;
    address++;
  }

  gEepromWrite = true;
}
