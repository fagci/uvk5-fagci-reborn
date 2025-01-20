#include "apps/apps.h"
#include "board.h"
#include "driver/audio.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "driver/crc.h"
#include "driver/eeprom.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "driver/systick.h"
#include "driver/uart.h"
#include "external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"
#include "external/FreeRTOS/include/FreeRTOS.h"
#include "external/FreeRTOS/include/timers.h"
#include "helper/bands.h"
#include "helper/battery.h"
#include "helper/channels.h"
#include "helper/lootlist.h"
#include "misc.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include "svc.h"
#include "task.h"
#include "ui/graphics.h"
#include "ui/spectrum.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

StaticTask_t systemTaskBuffer;
StackType_t systemTaskStack[configMINIMAL_STACK_SIZE + 100];

static void Boot(AppType_t appToRun) {
  hasSi = RADIO_HasSi();
  isPatchPresent = SETTINGS_IsPatchPresent();
  RADIO_SetupRegisters();

  SVC_Toggle(SVC_KEYBOARD, true, 10);
  // SVC_Toggle(SVC_LISTEN, true, 10);
  SVC_Toggle(SVC_APPS, true, 1);
  // SVC_Toggle(SVC_SYS, true, 1000);

  APPS_run(appToRun);
}

void _putchar(char c) { UART_Send((uint8_t *)&c, 1); }
void vAssertCalled(__attribute__((unused)) unsigned long ulLine,
                   __attribute__((unused)) const char *const pcFileName) {

  /* taskENTER_CRITICAL();
  { Log("[ASSERT ERROR] %s %s: line=%lu\r\n", __func__, pcFileName, ulLine); }
  taskEXIT_CRITICAL(); */
}

void vApplicationStackOverflowHook(__attribute__((unused)) TaskHandle_t pxTask,
                                   __attribute__((unused)) char *pcTaskName) {

  /* taskENTER_CRITICAL();
  {
    unsigned int stackWm = uxTaskGetStackHighWaterMark(pxTask);
    Log("[STACK ERROR] %s task=%s : %i\r\n", __func__, pcTaskName, stackWm);
  }
  taskEXIT_CRITICAL(); */
}
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
  static StaticTask_t xIdleTaskTCB;
  static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize) {
  static StaticTask_t xTimerTaskTCB;
  static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

static void Intro(void) {
  UI_ClearScreen();
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL, "r3b0rn");
  ST7565_Blit();

  BANDS_Load();
  if (gSettings.beep) {
    AUDIO_PlayTone(1400, 50);
  }

  /* UI_ClearScreen();
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL, "(^__^)");
  ST7565_Blit(); */

  Boot(gSettings.mainApp);

  if (gSettings.beep) {
    AUDIO_PlayTone(1400, 50);
  }
}

void mainTask() {
  LogUart("fagci R3b0rn\n");
  BOARD_Init();

  gSettings.contrast = 6;

  BACKLIGHT_SetBrightness(7);

  SVC_Toggle(SVC_RENDER, true, 25);

  KEY_Code_t pressedKey = KEYBOARD_Poll();

  /* UI_ClearScreen();
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL, "(X__X)");
  ST7565_Blit(); */

  uint8_t buf[2];
  uint8_t deadBuf[] = {0xDE, 0xAD};
  EEPROM_ReadBuffer(0, buf, 2);

  if (pressedKey == KEY_EXIT || memcmp(buf, deadBuf, 2) == 0) {
    gSettings.batteryCalibration = 2000;
    gSettings.backlight = 5;
    Boot(APP_RESET);
  } else {
    SETTINGS_Load();
    /* if (gSettings.batteryCalibration > 2154 ||
        gSettings.batteryCalibration < 1900) {
      gSettings.batteryCalibration = 0;
      EEPROM_WriteBuffer(0, deadBuf, 2);
      NVIC_SystemReset();
    } */

    // ST7565_Init();
    // BACKLIGHT_Init();

    BATTERY_UpdateBatteryInfo();
    Intro();
  }
  for (;;) {
    Log("-");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // SVC_Toggle(SVC_UART, true, 10);
}

Band b;
Measurement m = {};

uint32_t lm = 0;

void measure(TimerHandle_t xTimer) {
  m.rssi = BK4819_GetRSSI();
  m.snr = 0;
  SP_AddPoint(&m);

  if (Now() - lm > 120) {
    UI_ClearScreen();
    SP_Render(&b);
    ST7565_Blit();
    lm = Now();
  }

  m.f += StepFrequencyTable[b.step];
  if (m.f > b.txF) {
    m.f = b.rxF;
    SP_Begin();
  }
  BK4819_TuneTo(m.f, true);
}

void analyzer() {
  static StaticTimer_t _msmT;
  static TimerHandle_t msmT;

  b.rxF = 43307500;
  b.txF = 43307500 + 2500 * 128;
  b.step = 11; // 25k
  m.f = b.rxF;
  SP_Init(&b);

  BOARD_Init();
  gSettings.contrast = 6;
  BACKLIGHT_SetBrightness(7);

  RADIO_SetupRegisters();
  BK4819_SetFilterBandwidth(BK4819_FILTER_BW_12k);
  BK4819_SetAGC(true, 22);
  BK4819_TuneTo(b.rxF, true);
  msmT =
      xTimerCreateStatic("deb", pdMS_TO_TICKS(1), pdTRUE, 0, measure, &_msmT);
  xTimerStart(msmT, 0);

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void Main(void) {
  SYSTICK_Init();
  SYSTEM_ConfigureSysCon();
  BOARD_GPIO_Init();
  BOARD_PORTCON_Init();
  BOARD_ADC_Init();
  // CRC_Init();
  UART_Init();

  xTaskCreateStatic(analyzer, "Analyzer", ARRAY_SIZE(systemTaskStack), NULL, 1,
                    systemTaskStack, &systemTaskBuffer);

  vTaskStartScheduler();

  for (;;) {
  }

  xTaskCreateStatic(mainTask, "MAIN", ARRAY_SIZE(systemTaskStack), NULL, 1,
                    systemTaskStack, &systemTaskBuffer);

  while (true) {
    // TasksUpdate();
  }
}
