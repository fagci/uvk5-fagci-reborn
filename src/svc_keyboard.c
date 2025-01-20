#include "svc_keyboard.h"
#include "apps/apps.h"
#include "driver/audio.h"
#include "driver/backlight.h"
#include "driver/gpio.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "inc/dp32g030/gpio.h"
#include "scheduler.h"
#include "screencapture.h"
#include "settings.h"

static StaticTask_t taskBuffer;
static StackType_t taskStack[configMINIMAL_STACK_SIZE + 100];

// NOTE: Important!
// If app runs app on keypress, keyup passed to next app
// Common practice:
//
// keypress (up)
// keyhold
static void onKey(KEY_Code_t key, bool pressed, bool hold) {
  if (key != KEY_INVALID) {
    BACKLIGHT_On();
    // TaskTouch(BACKLIGHT_Update);
  }

  if (hold && pressed && !gRepeatHeld && key == KEY_F) {
    gSettings.keylock = !gSettings.keylock;
    SETTINGS_Save();
    gRedrawScreen = true;
    return;
  }

  if (gSettings.keylock && key == KEY_8 && hold && pressed) {
    captureScreen();
    return;
  }

  if (gSettings.keylock && (gSettings.pttLock ? true : key != KEY_PTT) &&
      !(hold && pressed && !gRepeatHeld && key == KEY_F)) {
    return;
  }

  // apps needed this events:
  // - keyup (!pressed)
  // - keyhold pressed (hold && pressed)
  // - keyup hold (hold && !pressed)
  if ((hold || !pressed) && APPS_key(key, pressed, hold)) {
    gRedrawScreen = true;
    return;
  }

  if (key == KEY_SIDE2 && !hold && !pressed) {
    GPIO_FlipBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
    return;
  }

  if (key != KEY_MENU) {
    return;
  }

  if (pressed) {
    if (hold) {
      APPS_run(APP_SETTINGS);
      return;
    }
  } else {
    if (!hold) {
      APPS_run(APP_APPS_LIST);
      return;
    }
  }
}

void SVC_KEYBOARD_Init(void) {
  xTaskCreateStatic(SVC_KEYBOARD_Update, "KBD", ARRAY_SIZE(taskStack), NULL, 2,
                    taskStack, &taskBuffer);
}

void SVC_KEYBOARD_Update(void) {
  for (;;) {
    Log("KBD .");
    KEYBOARD_CheckKeys(onKey);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void SVC_KEYBOARD_Deinit(void) {}
