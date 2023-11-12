#include "backlight.h"
#include "../inc/dp32g030/gpio.h"
#include "audio.h"
#include "gpio.h"

uint8_t duration = 2;
uint8_t countdown;
bool state = false;

void BACKLIGHT_Toggle(bool on) {
  if (state == on) {
    return;
  }
  state = on;
  if (on) {
    GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);
  } else {
    GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);
  }
}

void BACKLIGHT_On() {
  countdown = duration;
  BACKLIGHT_Toggle(true);
}

void BACKLIGHT_SetDuration(uint8_t durationSec) {
  duration = durationSec;
  countdown = durationSec;
}

void BACKLIGHT_Update() {
  if (countdown == 0 || countdown == 255) {
    return;
  }
  AUDIO_PlayTone(800, 50);

  if (countdown == 1) {
    BACKLIGHT_Toggle(false);
    countdown = 0;
  } else {
    countdown--;
  }
}
