#include "board.h"
#include "driver/audio.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "driver/systick.h"
#include "external/printf/printf.h"
#include "helper/battery.h"
#include "helper/measurements.h"
#include "inc/dp32g030/gpio.h"
#include "inc/dp32g030/syscon.h"
#include "misc.h"
#include "radio.h"
#include "scheduler.h"
#include "ui/components.h"
#include "ui/helper.h"
#include <string.h>

uint16_t rssi = 0;

void _putchar(char c) {}

struct CH {
  uint32_t t : 2;
};

void GetChannelName(uint8_t num, char *name) {
  memset(name, 0, 16);
  EEPROM_ReadBuffer(0x0F50 + (num * 0x10), name, 10);
}

void Render() {
  char String[16];
  memset(gStatusLine, 0, sizeof(gStatusLine));
  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  UI_Battery(gBatteryDisplayLevel);

  sprintf(String, "%u.%02uV", gBatteryVoltageAverage / 100,
          gBatteryVoltageAverage % 100);
  UI_PrintStringSmallest(String, 85, 1, true, true);

  // memset(gFrameBuffer[3], 3, Rssi2PX(rssi, 0, 128));

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
  AUDIO_ToggleSpeaker(on);

  if (on) {
    BK4819_SetAF(BK4819_AF_OPEN);
  } else {
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

void beep() {
  if (!rxState)
    AUDIO_PlayTone(1000, 20);
}

void Main(void) {
  SYSTEM_ConfigureSysCon();
  SYSTICK_Init();

  BOARD_Init();

  BATTERY_UpdateBatteryInfo();
  RADIO_SetupRegisters();

  GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);

  uint32_t f = 43400000;

  BK4819_TuneTo(f, true);
  BK4819_Squelch(3, f);

  TaskAdd(BATTERY_UpdateBatteryInfo, 5000, true);
  TaskAdd(CheckSquelch, 100, true);
  TaskAdd(Render, 500, true);
  TaskAdd(beep, 2000, true);

  while (1) {
    SYSTEM_DelayMs(100);
  }
}
