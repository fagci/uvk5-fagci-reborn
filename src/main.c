#include "apps/apps.h"
#include "board.h"
#include "driver/audio.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "driver/systick.h"
#include "driver/uart.h"
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

void _putchar(char c) {}

static void selfTest(void) {

  uint8_t buf[8];
  for (uint8_t i = 0; i < 8; ++i) {
    buf[i] = i;
  }
  EEPROM_WriteBuffer(0, buf, 8);
  EEPROM_WriteBuffer(262144 - 8, buf, 8);

  PrintSmall(0, 16, "Bytes are written");
  ST7565_Blit();

  while (true)
    continue;
}

static void unreborn(void) {
  uint8_t tpl[128];
  memset(tpl, 0xFF, 128);
  UI_ClearScreen();
  const uint32_t EEPROM_SIZE = SETTINGS_GetEEPROMSize();
  const uint8_t PAGE_SIZE = SETTINGS_GetPageSize();
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

// TODO:

// static void TX(void) {
// DEV = 300 for SSB
// SAVE 74, dev
// }

static void AddTasks(void) {
  SVC_Toggle(SVC_KEYBOARD, true, 10);
  SVC_Toggle(SVC_LISTEN, true, 10);
  SVC_Toggle(SVC_APPS, true, 1);
  SVC_Toggle(SVC_SYS, true, 1000);

  APPS_run(gSettings.mainApp);
}

static uint8_t introIndex = 0;
static void Intro(void) {
  char pb[] = "-\\|/";
  UI_ClearScreen();
  PrintMedium(4, 0 + 12, "OSFW");
  PrintMedium(16, 2 * 8 + 12, "reb0rn");
  PrintMedium(72, 2 * 8 + 12, "%c", pb[introIndex & 3]);
  PrintSmall(96, 46, "by fagci");
  ST7565_Blit();

  if (PRESETS_Load()) {
    if (gSettings.beep)
      AUDIO_PlayTone(1400, 50);

    TaskRemove(Intro);
    if (gSettings.beep)
      AUDIO_PlayTone(1400, 50);
    AddTasks();
    Log("SETTINGS %02u sz %02u", SETTINGS_OFFSET, SETTINGS_SIZE);
    Log("VFO1 %02u sz %02u", VFOS_OFFSET, VFO_SIZE);
    Log("VFO2 %02u sz %02u", VFOS_OFFSET + VFO_SIZE, VFO_SIZE);
    Log("PRESET %02u sz %02u", PRESETS_OFFSET, PRESET_SIZE);
    Log("P22 BW: %u", PRESETS_Item(22)->band.bw);

    RADIO_LoadCurrentVFO();
  }
}

void Main(void) {
  SYSTICK_Init();
  SYSTEM_ConfigureSysCon();

  BOARD_Init();
  BACKLIGHT_Toggle(true);
  SVC_Toggle(SVC_RENDER, true, 25);
  KEY_Code_t pressedKey = KEYBOARD_Poll();
  if (pressedKey == KEY_EXIT) {
    BACKLIGHT_SetDuration(120);
    BACKLIGHT_SetBrightness(15);
    BACKLIGHT_On();
    reset();
  } else if (pressedKey == KEY_7) {
    BACKLIGHT_SetDuration(120);
    BACKLIGHT_SetBrightness(15);
    BACKLIGHT_On();
    unreborn();
  }

  SETTINGS_Load();
  // gSettings.eepromType = EEPROM_M24M02;

  /* if (gSettings.checkbyte != EEPROM_CHECKBYTE) {
    gSettings.eepromType = EEPROM_BL24C64;
    BACKLIGHT_SetDuration(120);
    BACKLIGHT_SetBrightness(15);
    BACKLIGHT_On();
    reset();
  } */

  UART_Init();

  BATTERY_UpdateBatteryInfo();
  RADIO_SetupRegisters();

  BACKLIGHT_SetDuration(BL_TIME_VALUES[gSettings.backlight]);
  BACKLIGHT_SetBrightness(gSettings.brightness);
  BACKLIGHT_On();

  if (KEYBOARD_Poll() == KEY_STAR) {
    PrintMediumEx(0, 7, POS_L, C_FILL, "SET: %u %u", SETTINGS_OFFSET,
                  SETTINGS_SIZE);
    PrintMediumEx(0, 7 + 8, POS_L, C_FILL, "VFO: %u %u", VFOS_OFFSET, VFO_SIZE);
    PrintMediumEx(0, 7 + 16, POS_L, C_FILL, "PRES CNT: %u",
                  gSettings.presetsCount);
    ST7565_Blit();
  } else if (KEYBOARD_Poll() == KEY_F) {
    UART_IsLogEnabled = 5;
    TaskAdd("Intro", Intro, 2, true, 5);
  } else if (KEYBOARD_Poll() == KEY_MENU) {
    selfTest();
    /* PrintMediumEx(LCD_WIDTH - 1, 7, POS_R, C_FILL, "%u", PRESETS_Size());
    for (uint8_t i = 0; i < PRESETS_Size(); ++i) {
      Preset p;
      PRESETS_LoadPreset(i, &p);
      PrintSmall(i / 10 * 40, 6 * (i % 10) + 6, "%u - %u",
                 p.band.bounds.start / 100000, p.band.bounds.end / 100000);
    }
    ST7565_Blit(); */
  } else if (KEYBOARD_Poll() == KEY_5) {
    SVC_Toggle(SVC_KEYBOARD, true, 10);
    SVC_Toggle(SVC_LISTEN, true, 10);
    SVC_Toggle(SVC_APPS, true, 1);
    SVC_Toggle(SVC_SYS, true, 1000);

    APPS_run(APP_TEST);
  } else {
    TaskAdd("Intro", Intro, 2, true, 5);
  }

  while (true) {
    TasksUpdate(); // TODO: check if delay not needed or something
  }
}
