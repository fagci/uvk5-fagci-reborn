#include "bk1080.h"
#include "../driver/uart.h"
#include "../inc/dp32g030/gpio.h"
#include "../misc.h"
#include "bk1080-regs.h"
#include "gpio.h"
#include "i2c.h"
#include "system.h"

static const uint16_t BK1080_RegisterTable[] = {
    0x0008, 0x1080, 0x0201, 0x0000, 0x40C0, 0x0A1F, 0x002E, 0x02FF, 0x5B11,
    0x0000, 0x411E, 0x0000, 0xCE00, 0x0000, 0x0000, 0x1000, 0x3197, 0x0000,
    0x13FF, 0x9852, 0x0000, 0x0000, 0x0008, 0x0000, 0x51E1, 0xA8BC, 0x2645,
    0x00E4, 0x1CD8, 0x3A50, 0xEAE0, 0x3000, 0x0200, 0x0000,
};

static bool gIsInitBK1080;
static uint32_t currentF = 0;

uint16_t CH_SP_F[] = {20000, 10000, 5000};

void BK1080_SetFrequency(uint32_t f) {
  if (f == currentF) {
    return;
  }
  currentF = f;
  uint8_t vol = 0b1111;
  uint8_t chSp = BK1080_CHSP_100;
  uint8_t seekThres = 0b00001010;

  uint8_t band = f < 7600000 ? BK1080_BAND_64_76 : BK1080_BAND_76_108;

  uint32_t startF = band == BK1080_BAND_64_76 ? 6400000 : 7600000;

  uint16_t channel = (f - startF) / CH_SP_F[chSp];

  uint16_t sysCfg2 = (vol << 0) | (chSp << 4) | (band << 6) | (seekThres << 8);

  BK1080_WriteRegister(BK1080_REG_05_SYSTEM_CONFIGURATION2, sysCfg2);
  BK1080_WriteRegister(BK1080_REG_03_CHANNEL, channel);
  SYSTEM_DelayMs(10);
  BK1080_WriteRegister(BK1080_REG_03_CHANNEL, channel | 0x8000);
}

void BK1080_Init(uint32_t f, bool bEnable) {
  uint8_t i;

  if (bEnable) {
    GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BK1080);

    if (!gIsInitBK1080) {
      for (i = 0; i < ARRAY_SIZE(BK1080_RegisterTable); i++) {
        BK1080_WriteRegister(i, BK1080_RegisterTable[i]);
      }
      SYSTEM_DelayMs(250);
      BK1080_WriteRegister(BK1080_REG_25_INTERNAL, 0xA83C);
      BK1080_WriteRegister(BK1080_REG_25_INTERNAL, 0xA8BC);
      SYSTEM_DelayMs(60);
      gIsInitBK1080 = true;
    } else {
      BK1080_WriteRegister(BK1080_REG_02_POWER_CONFIGURATION, 0x0201);
    }
    currentF = 0;
    BK1080_SetFrequency(f);
  } else {
    BK1080_WriteRegister(BK1080_REG_02_POWER_CONFIGURATION, 0x0241);
    GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BK1080);
  }
}

uint16_t BK1080_ReadRegister(BK1080_Register_t Register) {
  uint8_t Value[2];

  I2C_Start();
  I2C_Write(0x80);
  I2C_Write((Register << 1) | I2C_READ);
  I2C_ReadBuffer(Value, sizeof(Value));
  I2C_Stop();
  return (Value[0] << 8) | Value[1];
}

void BK1080_WriteRegister(BK1080_Register_t Register, uint16_t Value) {
  I2C_Start();
  I2C_Write(0x80);
  I2C_Write((Register << 1) | I2C_WRITE);
  Value = ((Value >> 8) & 0xFF) | ((Value & 0xFF) << 8);
  I2C_WriteBuffer(&Value, sizeof(Value));
  I2C_Stop();
}

void BK1080_Mute(bool Mute) {
  if (Mute) {
    BK1080_WriteRegister(BK1080_REG_02_POWER_CONFIGURATION, 0x4201);
  } else {
    BK1080_WriteRegister(BK1080_REG_02_POWER_CONFIGURATION, 0x0201);
  }
}

uint16_t BK1080_GetFrequencyDeviation() {
  return BK1080_ReadRegister(BK1080_REG_07) >> 4;
}

uint16_t BK1080_GetRSSI() {
  return (BK1080_ReadRegister(BK1080_REG_10) & 0xFF) << 1;
}
