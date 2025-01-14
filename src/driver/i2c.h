#ifndef DRIVER_I2C_H
#define DRIVER_I2C_H

#include <stdbool.h>
#include <stdint.h>

enum {
  I2C_WRITE = 0U,
  I2C_READ = 1U,
};

void I2C_Start(void);
void I2C_Stop(void);

uint8_t I2C_Read(bool bFinal);
int I2C_Write(uint8_t Data);

uint16_t I2C_ReadBuffer(void *pBuffer, uint16_t Size);
uint16_t I2C_WriteBuffer(const void *pBuffer, uint16_t Size);

#endif
