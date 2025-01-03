#include "apps/apps.h"
#include "board.h"
#include "driver/audio.h"
#include "driver/backlight.h"
#include "driver/eeprom.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "driver/systick.h"
#include "driver/uart.h"
#include "external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"
#include "helper/bands.h"
#include "helper/battery.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include "svc.h"
#include "ui/graphics.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static void Boot(AppType_t appToRun) {
  hasSi = RADIO_HasSi();
  isPatchPresent = SETTINGS_IsPatchPresent();
  RADIO_SetupRegisters();

  SVC_Toggle(SVC_KEYBOARD, true, 10);
  SVC_Toggle(SVC_LISTEN, true, 0);
  SVC_Toggle(SVC_APPS, true, 1);
  SVC_Toggle(SVC_SYS, true, 1000);

  APPS_run(appToRun);
}

void _putchar(char c) { UART_Send((uint8_t *)&c, 1); }

void uartHandle() {
  if (UART_IsCommandAvailable()) {
    __disable_irq();
    UART_HandleCommand();
    __enable_irq();
  }
}

static void Intro(void) {
  UI_ClearScreen();
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL, "r3b0rn");
  ST7565_Blit();

  BANDS_Load();
  if (gSettings.beep) {
    AUDIO_PlayTone(1400, 50);
  }

  UI_ClearScreen();
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL, "(^__^)");
  ST7565_Blit();

  TaskRemove(Intro);

  Boot(gSettings.mainApp);

  if (gSettings.beep) {
    AUDIO_PlayTone(1400, 50);
  }
}

void Main(void) {
  SYSTEM_ConfigureSysCon();
  SYSTICK_Init();

  gSettings.contrast = 6;
  BOARD_Init();

  BACKLIGHT_SetBrightness(7);

  SVC_Toggle(SVC_RENDER, true, 25);

  KEY_Code_t pressedKey = KEYBOARD_Poll();

  UI_ClearScreen();
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL, "(X__X)");
  ST7565_Blit();

  uint8_t buf[2];
  uint8_t deadBuf[] = {0xDE, 0xAD};
  EEPROM_ReadBuffer(0, buf, 2);

  if (pressedKey > KEY_0 && pressedKey < KEY_7) {
    gSettings.batteryCalibration = 2000;
    gSettings.eepromType = pressedKey - 1;
    gSettings.backlight = 5;
    Boot(APP_MEMVIEW);
  } else if (pressedKey == KEY_EXIT || memcmp(buf, deadBuf, 2) == 0) {
    gSettings.batteryCalibration = 2000;
    gSettings.backlight = 5;
    Boot(APP_RESET);
  } else {
    SETTINGS_Load();
    if (gSettings.batteryCalibration > 2154 ||
        gSettings.batteryCalibration < 1900) {
      gSettings.batteryCalibration = 0;
      EEPROM_WriteBuffer(0, deadBuf, 2);
      NVIC_SystemReset();
    }

    ST7565_Init();
    BACKLIGHT_Init();

    BATTERY_UpdateBatteryInfo();
    TaskAdd("Intro", Intro, 1, true, 5);
  }

  TaskAdd("UART", uartHandle, 100, true, 0);

  while (true) {
    TasksUpdate();
  }
}
