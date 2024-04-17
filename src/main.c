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
#include "helper/battery.h"
#include "helper/presetlist.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include "svc.h"
#include "ui/graphics.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static void Boot(AppType_t appToRun) {
  SVC_Toggle(SVC_KEYBOARD, true, 10);
  SVC_Toggle(SVC_LISTEN, true, 10);
  SVC_Toggle(SVC_APPS, true, 1);
  SVC_Toggle(SVC_SYS, true, 1000);

  APPS_run(appToRun);
}

void _putchar(char c) {
  // UART_Send((uint8_t *)&c, 1);
}

static void unreborn(void) {
  uint8_t tpl[128];
  const uint32_t EEPROM_SIZE = SETTINGS_GetEEPROMSize();
  const uint8_t PAGE_SIZE = SETTINGS_GetPageSize();

  memset(tpl, 0xFF, 128);

  for (uint16_t i = 0; i < EEPROM_SIZE; i += PAGE_SIZE) {
    EEPROM_WriteBuffer(i, tpl, PAGE_SIZE);
    UI_ClearScreen();
    PrintMediumEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL, "0xFFing... %u",
                  i * 100 / EEPROM_SIZE);
    ST7565_Blit();
  }

  UI_ClearScreen();
  PrintMediumEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL, "0xFFed !!!");
  ST7565_Blit();

  while (true)
    continue;
}

static void reset(void) {
  SVC_Toggle(SVC_APPS, true, 1);
  APPS_run(APP_RESET);
  while (true) {
    TasksUpdate();
  }
}

static void Intro(void) {
  UI_ClearScreen();
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL, "r3b0rn");
  ST7565_Blit();

  if (PRESETS_Load()) {
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
}

void Main(void) {
  gSettings.contrast = 6;
  SYSTEM_ConfigureSysCon();
  SYSTICK_Init();
  BOARD_Init();
  UART_Init();

  BACKLIGHT_SetBrightness(7);

  SVC_Toggle(SVC_RENDER, true, 25);

  KEY_Code_t pressedKey = KEYBOARD_Poll();
  if (pressedKey == KEY_EXIT) {
    reset();
  } else if (pressedKey == KEY_7) {
    unreborn();
  }

  SETTINGS_Load();

  BATTERY_UpdateBatteryInfo();
  RADIO_SetupRegisters();

  BACKLIGHT_SetDuration(BL_TIME_VALUES[gSettings.backlight]);
  BACKLIGHT_SetBrightness(gSettings.brightness);
  BACKLIGHT_On();
  ST7565_Init();

  if (KEYBOARD_Poll() == KEY_5) {
    Boot(APP_MEMVIEW);
  } else {
    TaskAdd("Intro", Intro, 1, true, 5);
  }

  while (true) {
    TasksUpdate(); // TODO: check if delay not needed or something
  }
}
