/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include "../driver/eeprom.h"
#include "../driver/gpio.h"
#include "../driver/i2c.h"
#include "../driver/system.h"
#include "../inc/dp32g030/gpio.h"

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
}

void EEPROM_WriteBuffer(uint16_t address, const void *pBuffer, uint8_t size) {
  GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
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
  GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
}
