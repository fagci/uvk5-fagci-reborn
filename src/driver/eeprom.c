#include "../driver/eeprom.h"
#include "../driver/i2c.h"
#include "../driver/system.h"
#include "../settings.h"
#include "ARMCM0.h"
#include <stddef.h>
#include <string.h>

bool gEepromWrite = false;
bool gEepromRead = false;

void EEPROM_ReadBuffer(uint32_t address, void *pBuffer, uint8_t size) {
  uint8_t IIC_ADD = (uint8_t)(0xA0 | ((address / 0x10000) << 1));

  if (gSettings.eepromType == EEPROM_M24M02) {
    if (address >= 0x40000) {
      IIC_ADD = (uint8_t)(0xA8 | (((address - 0x40000) / 0x10000) << 1));
      address -= 0x40000;
    }
  }

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

  gEepromRead = true;
}

// static uint8_t tmpBuffer[256];
void EEPROM_WriteBuffer(uint32_t address, void *pBuffer, uint8_t size) {
  if (pBuffer == NULL) {
    return;
  }
  const uint8_t PAGE_SIZE = SETTINGS_GetPageSize();

  while (size) {
    uint32_t pageNum = address / PAGE_SIZE;
    uint32_t rest = (pageNum + 1) * PAGE_SIZE - address;

    // TODO: assume that size < PAGE_SIZE
    uint8_t n = rest > size ? size : (uint8_t)rest;

    /* EEPROM_ReadBuffer(address, tmpBuffer, n);
    if (memcmp(buf, tmpBuffer, n) != 0) { */
    uint8_t IIC_ADD = (uint8_t)(0xA0 | ((address / 0x10000) << 1));

    if (gSettings.eepromType == EEPROM_M24M02) {
      if (address >= 0x40000) {
        IIC_ADD = (uint8_t)(0xA8 | (((address - 0x40000) / 0x10000) << 1));
      }
    }

    __disable_irq();
    I2C_Start();
    I2C_Write(IIC_ADD);
    I2C_Write((address >> 8) & 0xFF);
    I2C_Write(address & 0xFF);

    I2C_WriteBuffer(pBuffer, n);

    I2C_Stop();
    __enable_irq();
    SYSTEM_DelayMs(8);

    pBuffer += n;
    address += n;
    size -= n;
    // }
    gEepromWrite = true;
  }
}
