#include "audio.h"
#include "board.h"
#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/syscon.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "driver/gpio.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "driver/systick.h"
#include "helper/battery.h"
#include "helper/measurements.h"
#include "misc.h"
#include "radio.h"
#include "scheduler.h"
#include "ui/components.h"
#include <string.h>

uint8_t v1 = 0;
uint8_t v2 = 0;
uint16_t rssi = 0;

void Render() {
  memset(gStatusLine, 0, sizeof(gStatusLine));
  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  UI_Battery(gBatteryDisplayLevel);

  uint16_t Voltage = Mid(gBatteryVoltages, ARRAY_SIZE(gBatteryVoltages));
  for (uint8_t i = 0; i < Voltage / 100; i++) {
    gFrameBuffer[5][i*3] |= 0b00000010;
  }
  for (uint8_t i = 0; i < Voltage / 10 % 10; i++) {
    gFrameBuffer[5][i*3] |= 0b00001000;
  }
  for (uint8_t i = 0; i < Voltage % 10; i++) {
    gFrameBuffer[5][i*3] |= 0b00100000;
  }

  memset(gFrameBuffer[3], 3, Rssi2PX(rssi, 0, 128));

  ST7565_BlitStatusLine();
  ST7565_BlitFullScreen();
}

bool rxState = false;

void ToggleRX(bool on) {
  if (on == rxState) {
    return;
  }
  rxState = on;
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_GREEN, on);
  if (on) {
    GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
    BK4819_SetAF(BK4819_AF_OPEN);
    // BK4819_RX_TurnOn();
  } else {
    GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
    BK4819_SetAF(BK4819_AF_MUTE);
  }
}

void CheckSquelch() {
  rssi = BK4819_GetRSSI();
  while (BK4819_ReadRegister(BK4819_REG_0C) & 1U) {
    BK4819_WriteRegister(BK4819_REG_02, 0);

    uint16_t Mask = BK4819_ReadRegister(BK4819_REG_02);
    bool rx = false;

    if (Mask & BK4819_REG_02_SQUELCH_LOST) {
      rx = true;
    }

    if (Mask & BK4819_REG_02_SQUELCH_FOUND) {
      rx = false;
    }
    ToggleRX(rx);
  }
}

void Main(void) {
  // Enable clock gating of blocks we need.
  SYSCON_DEV_CLK_GATE = 0 | SYSCON_DEV_CLK_GATE_GPIOA_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_GPIOB_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_GPIOC_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_UART1_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_SPI0_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_SARADC_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_CRC_BITS_ENABLE |
                        SYSCON_DEV_CLK_GATE_AES_BITS_ENABLE;

  SYSTICK_Init();
  BOARD_Init();

  BK4819_Init();

  BATTERY_UpdateBatteryInfo();
  RADIO_SetupRegisters();

  GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);

  BK4819_TuneTo(43400000, true);

  TaskAdd(BATTERY_UpdateBatteryInfo, 5000, true);
  TaskAdd(CheckSquelch, 100, true);
  TaskAdd(Render, 500, true);

  while (1) {
    SYSTEM_DelayMs(100);
  }
}
