#include "../driver/eeprom.h"
#include "../driver/i2c.h"
#include "../driver/system.h"
// #include "../external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"
#include "../settings.h"
#include <stddef.h>
#include <string.h>

bool gEepromWrite = false;

void EEPROM_ReadBuffer(uint32_t address, void *pBuffer, uint16_t size) {
  uint8_t IIC_ADD = (uint8_t)(0xA0 | ((address / 0x10000) << 1));

  I2C_Start();
  I2C_Write(IIC_ADD);
  I2C_Write((address >> 8) & 0xFF);
  I2C_Write(address & 0xFF);

  I2C_Start();
  I2C_Write(IIC_ADD + 1);
  I2C_ReadBuffer(pBuffer, size);
  I2C_Stop();
}

static uint8_t tmpBuffer[128];
void EEPROM_WriteBuffer(uint32_t address, void *pBuffer, uint16_t size) {
  if (pBuffer == NULL) {
    return;
  }
  const uint8_t PAGE_SIZE = SETTINGS_GetPageSize();

  while (size) {
    uint16_t i = address % PAGE_SIZE;
    uint16_t rest = PAGE_SIZE - i;
    uint16_t n = size < rest ? size : rest;

    EEPROM_ReadBuffer(address, tmpBuffer, n);
    if (memcmp(pBuffer, tmpBuffer, n) != 0) {
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
    gEepromWrite = true;
  }
}
