#include "i2c.h"
#include "../external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"
#include "../inc/dp32g030/gpio.h"
#include "../inc/dp32g030/portcon.h"
#include "gpio.h"
#include "systick.h"

void I2C_Start(void) {
  __disable_irq();
  GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
  SYSTICK_DelayTicks(16);
  GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
  SYSTICK_DelayTicks(16);
  GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
  SYSTICK_DelayTicks(16);
  GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
  SYSTICK_DelayTicks(16);
}

void I2C_Stop(void) {
  GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
  SYSTICK_DelayTicks(16);
  GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
  SYSTICK_DelayTicks(16);
  GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
  SYSTICK_DelayTicks(16);
  GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
  SYSTICK_DelayTicks(16);
  __enable_irq();
}

uint8_t I2C_Read(bool bFinal) {
  uint8_t i, Data;

  PORTCON_PORTA_IE |= PORTCON_PORTA_IE_A11_BITS_ENABLE;
  PORTCON_PORTA_OD &= ~PORTCON_PORTA_OD_A11_MASK;
  GPIOA->DIR &= ~GPIO_DIR_11_MASK;

  Data = 0;
  for (i = 0; i < 8; i++) {
    GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
    SYSTICK_DelayTicks(16);
    GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
    SYSTICK_DelayTicks(16);
    Data <<= 1;
    SYSTICK_DelayTicks(16);
    if (GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA)) {
      Data |= 1U;
    }
    GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
    SYSTICK_DelayTicks(16);
  }

  PORTCON_PORTA_IE &= ~PORTCON_PORTA_IE_A11_MASK;
  PORTCON_PORTA_OD |= PORTCON_PORTA_OD_A11_BITS_ENABLE;
  GPIOA->DIR |= GPIO_DIR_11_BITS_OUTPUT;
  GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
  SYSTICK_DelayTicks(16);
  if (bFinal) {
    GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
  } else {
    GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
  }
  SYSTICK_DelayTicks(16);
  GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
  SYSTICK_DelayTicks(16);
  GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
  SYSTICK_DelayTicks(16);

  return Data;
}

int I2C_Write(uint8_t Data) {
  uint8_t i;
  int ret = -1;

  GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
  SYSTICK_DelayTicks(16);
  for (i = 0; i < 8; i++) {
    if ((Data & 0x80) == 0) {
      GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
    } else {
      GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
    }
    Data <<= 1;
    SYSTICK_DelayTicks(16);
    GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
    SYSTICK_DelayTicks(16);
    GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
    SYSTICK_DelayTicks(16);
  }

  PORTCON_PORTA_IE |= PORTCON_PORTA_IE_A11_BITS_ENABLE;
  PORTCON_PORTA_OD &= ~PORTCON_PORTA_OD_A11_MASK;
  GPIOA->DIR &= ~GPIO_DIR_11_MASK;
  GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);
  SYSTICK_DelayTicks(16);
  GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
  SYSTICK_DelayTicks(16);

  for (i = 0; i < 255; i++) {
    if (GPIO_CheckBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA) == 0) {
      ret = 0;
      break;
    }
  }

  GPIO_ClearBit(&GPIOA->DATA, GPIOA_PIN_I2C_SCL);
  SYSTICK_DelayTicks(16);
  PORTCON_PORTA_IE &= ~PORTCON_PORTA_IE_A11_MASK;
  PORTCON_PORTA_OD |= PORTCON_PORTA_OD_A11_BITS_ENABLE;
  GPIOA->DIR |= GPIO_DIR_11_BITS_OUTPUT;
  GPIO_SetBit(&GPIOA->DATA, GPIOA_PIN_I2C_SDA);

  return ret;
}

uint16_t I2C_ReadBuffer(void *pBuffer, uint16_t Size) {
  uint8_t *pData = (uint8_t *)pBuffer;
  uint16_t i;

  for (i = 0; i < Size - 1; i++) {
    SYSTICK_DelayTicks(16);
    pData[i] = I2C_Read(false);
  }

  SYSTICK_DelayTicks(16);
  pData[i++] = I2C_Read(true);

  return Size;
}

uint16_t I2C_WriteBuffer(const void *pBuffer, uint16_t Size) {
  const uint8_t *pData = (const uint8_t *)pBuffer;
  uint16_t i;

  for (i = 0; i < Size; i++) {
    if (I2C_Write(*pData++) < 0) {
      return -1;
    }
  }

  return 0;
}
